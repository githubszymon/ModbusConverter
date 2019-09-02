#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include </home/pi/libmodbus/src/modbus.h>
#include <sys/socket.h>

#include <wiringPi.h>          
#include <lcd.h>     

#define SERVER_ID 1
#define TRANSACTION_ID_SIZE 2

#define MODBUS_TCP_HEADER_LENGHT 6
#define MODBUS_RTU_CRC_LENGHT 2

#define MODBUS_TCP_SLAVE_POSITION 6

#define LCD_RS  29               
#define LCD_E   28         
#define LCD_D4  27               
#define LCD_D5  26               
#define LCD_D6 6              
#define LCD_D7  5            

/* For MinGW */
#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

enum {
    TCP,
    TCP_PI,
    RTU
};

int init(modbus_t **ctx_tcp, modbus_t **ctx_rtu,int lcd)
{
    #define CHAR_PER_LINE 255

    FILE *fp;
    char str[CHAR_PER_LINE];
    char* filename = "parameters.conf";
    char *pach;

    char ip[]="                ";
    int port;
    int baund;
    char parity;
    int data;
    int stop;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Could not open init file %s",filename);
        return (1);
    }

    fgets(str, CHAR_PER_LINE, fp);
	strcpy(ip,str);
	ip[strlen(ip)-1]=0;

    fgets(str, CHAR_PER_LINE, fp);
    port=atoi(str);

    fgets(str, CHAR_PER_LINE, fp);
    baund=atoi(str);

    fgets(str, CHAR_PER_LINE, fp);
    parity=*str;

    fgets(str, CHAR_PER_LINE, fp);
    data=atoi(str);
     
    fgets(str, CHAR_PER_LINE, fp);
    stop=atoi(str);

    fclose(fp);

	*ctx_tcp = modbus_new_tcp(ip, port);
    *ctx_rtu = modbus_new_rtu("/dev/ttyAMA0", baund, parity, data, stop);

	lcdClear(lcd);
	lcdPrintf(lcd,"%s",ip);

	lcdPosition(lcd,0,1);
    lcdPrintf(lcd,"%d %c %d %d",baund,parity,data,stop);
	
    sleep(1);
    
    modbus_set_debug(*ctx_tcp, TRUE);
    modbus_set_debug(*ctx_rtu, TRUE);

    return(0);
}

int init_display()
{
    int lcd;               
    wiringPiSetup();        
    lcd = lcdInit (2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);
    lcdClear(lcd);
    lcdPuts(lcd, "MODBUS converter"); 
    sleep(1);
    return(lcd); 
}

int exit_clear (modbus_t **ctx_tcp, modbus_t **ctx_rtu)
{
    modbus_close(*ctx_rtu);
    modbus_free(*ctx_rtu);
    return(0);
}

int min(int num1, int num2)
{
    return(num1<num2)?num1:num2;
}

int main(int argc, char*argv[])
{
    int s = -1;
    
    modbus_t *ctx_tcp;
    modbus_t *ctx_rtu;
    uint8_t transaction_id[TRANSACTION_ID_SIZE];
    uint8_t *query_tcp, *query_rtu;

    int rc;
    int rc_tcp;
    int i;

    int lcd;
	
    char buffer[10];
    
    lcd=init_display();
    init(&ctx_tcp,&ctx_rtu,lcd);

    query_tcp = malloc(MODBUS_TCP_MAX_ADU_LENGTH);
    query_rtu = malloc(MODBUS_RTU_MAX_ADU_LENGTH);

    s = modbus_tcp_listen(ctx_tcp, 1);
    modbus_tcp_accept(ctx_tcp, &s);
    
    if (modbus_connect(ctx_rtu) == -1) 
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx_rtu);
        return -1;
    }
           
    for (;;)
    {
        do 
        {
            rc_tcp = modbus_receive(ctx_tcp, query_tcp);
        } 
        while (rc_tcp == 0);

        if (rc_tcp == -1 && errno != EMBBADCRC) 
        {
            break;
        }
      
        modbus_set_slave(ctx_rtu, query_tcp[MODBUS_TCP_SLAVE_POSITION]);

	    lcdClear(lcd);
         
        for (i = 0; i < min (rc_tcp - MODBUS_TCP_HEADER_LENGHT,8); i++) 
        {
            sprintf(buffer,"%.2X", query_tcp[i + MODBUS_TCP_HEADER_LENGHT]);
            lcdPuts(lcd,buffer);
        }          
        
        modbus_send_raw_request(ctx_rtu, query_tcp + MODBUS_TCP_HEADER_LENGHT , (rc_tcp - MODBUS_TCP_HEADER_LENGHT) * sizeof(uint8_t));
        rc = modbus_receive_confirmation(ctx_rtu, query_rtu);  

        lcdPosition(lcd,0,1);
        for (i = 0; i < min(rc-MODBUS_RTU_CRC_LENGHT,8); i++) 
        {
            sprintf(buffer,"%.2X", query_rtu[i]);
            lcdPuts(lcd,buffer);
        }          

        for(i = 0;i <  rc - MODBUS_RTU_CRC_LENGHT ; i++) 
        {
            query_tcp[i + MODBUS_TCP_HEADER_LENGHT] = query_rtu[i];
        }    

        rc_tcp=rc + MODBUS_TCP_HEADER_LENGHT - MODBUS_RTU_CRC_LENGHT;

        int w_s = modbus_get_socket(ctx_tcp);
        int sent;
	   
        for (i = 0; i < rc_tcp; i++) {
			usleep(5000);
                    sent = send(w_s, (const char*)(query_tcp + i), 1, MSG_NOSIGNAL);
                    if (sent == -1) {
                        break;
                    }
        }            
        if (rc_tcp == -1) {
            break;
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    if (s != -1) {
        close(s);
    }

    free(query_tcp);
    free(query_rtu);
    
    exit_clear(&ctx_tcp,&ctx_rtu);

    return 0;
}
