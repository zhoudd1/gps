#include <nmea/nmea.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "nrf_gpio.h"
#include "GUI.h"
#include "ls013b7dh03.h"

#include "gps_main.h"
#include "main.h"

#define GPS_PIN   27 

static struct gps_t{
	char buf[100];
	uint8_t index;
	enum gps_state_t{
		 GPS_NULL,
		 START,
		 PAYLOAD,
		 END,
	}gps_state;
}gps;

void gps_on(void)
{
	nrf_gpio_cfg_output(GPS_PIN);
	nrf_gpio_pin_set(GPS_PIN);
	memset(gps.buf,0,sizeof(char)*100);
	gps.gps_state = START;
	gps.index = 0;
}

void gps_off(void)
{
	nrf_gpio_pin_clear(GPS_PIN);
}

void gps_buf_fill_data(uint8_t cr)
{
	switch (gps.gps_state)
	{
		case START:
				 if(cr == '$'){
						gps.buf[gps.index++] = cr;
						gps.gps_state = PAYLOAD;
				 }
				 else gps.gps_state = START;
			break;
				 
		case PAYLOAD:				
				if(cr == '\r'){								
					gps.gps_state = END;
				}								
				gps.buf[gps.index++] = cr;												
			break;
		
		case END: 
				if(cr == '\n'){								
				if( xSemaphoreGive( gpsSemaphore ) != pdTRUE )
				 {
				 }
					gps.buf[gps.index] = cr;
				}			
			break;
									
		default:
			break;
	}	
}

void gps_thread(void * arg)
{		
	  nmeaINFO info;
    nmeaPARSER parser;
	
    int size_nmeaINFO  = sizeof(nmeaINFO);
		int size_nmeaPARSER  = sizeof(nmeaPARSER);	
		
    nmea_zero_INFO(&info);
 		nmea_parser_init(&parser);
	
		static uint8_t gps_nmea_scanning = 0;
	  static int inuse_value = 0;
		static int inview_value = 0;
				
	  gps_on();
	
		for (;;)
		{					
			  if( xSemaphoreTake( gpsSemaphore, ( TickType_t ) 0 ) )
				{					
						switch (gps_nmea_scanning)
						{
							case 0:	
								if(strstr(gps.buf,"$GPRMC")!=NULL){
										if ((nmea_parse(&parser, (char*)gps.buf, (int)strlen((char*)gps.buf), &info)) > 0 )
										{		
											 /*
												printf("inuse     %d\r\n",inuse_value);
												printf("inview    %d\r\n",inview_value);											
												printf("sig       %d\r\n",info.sig);
												printf("fix       %d\r\n",info.fix);																				
												printf("year      %d\r\n",info.utc.year);
												printf("mon       %d\r\n",info.utc.mon);
												printf("day       %d\r\n",info.utc.day);										
												printf("hour      %d\r\n",info.utc.hour);
												printf("min       %d\r\n",info.utc.min);
												printf("sec       %d\r\n",info.utc.sec);																
												printf("lon       %.5f\r\n",info.lon);
												printf("lat       %.5f\r\n",info.lat);
												printf("speed     %.2f\r\n",info.speed);	
											*/																				
												GUI_GotoXY(0,16);									
												GUI_DispString("date:");	
												GUI_DispDec(info.utc.year+1900,4);GUI_DispString("/");GUI_DispDec(info.utc.mon,2);GUI_DispString("/");GUI_DispDec(info.utc.day,2);
																								
												GUI_GotoXY(0,32);									
												GUI_DispString("time:");									
												GUI_DispDec(info.utc.hour,2);GUI_DispString(":");GUI_DispDec(info.utc.min,2);GUI_DispString(":");GUI_DispDec(info.utc.sec,2);
																						
												GUI_GotoXY(0,48);									
												GUI_DispString("sig:");	
												GUI_DispDecMin(info.sig);												
												
												GUI_GotoXY(0,64);									
												GUI_DispString("fix:");
												GUI_DispDecMin(info.fix);												
												
												GUI_GotoXY(56,48);									
												GUI_DispString("inuse:");	
												GUI_DispDecMin(inuse_value);
																						
												GUI_GotoXY(56,64);									
												GUI_DispString("inview:");
												GUI_DispDecMin(inview_value);
												
												GUI_GotoXY(0,80);									
												GUI_DispString("lon:");
												GUI_DispFloatFix(info.lon,9,3);	
												
												GUI_GotoXY(0,96);
												GUI_DispString("lat:");
												GUI_DispFloatFix(info.lat,9,3);									
												
												GUI_GotoXY(0,112);
												GUI_DispString("speed:");
												GUI_DispFloatFix(info.speed,9,2);	

												lcd_refresh();																							 
										}
										gps_nmea_scanning = 1;
									}																			
								break;
							
							case 1:
									if(strstr(gps.buf,"$GPGGA")!=NULL){
									   /*
										 MTK的GPS模组GPGGA协议内容是14项，nmealib库是解析的12项，这里手动解算inuse
										 if ((nmea_parse(&parser, (char*)gps.buf, (int)strlen((char*)gps.buf), &info)) > 0 )
									   */	
											char inuse_buf[3];
											int count = 0, index = 0;
											for (index = 0; index < strlen(gps.buf); ++index){
												if (gps.buf[index] == ',')
													++count;
												if (count == 7)
													break;
											}
											strncpy(inuse_buf, &gps.buf[index+1], 2);
											int n = atoi(inuse_buf);
											inuse_value = n;
											gps_nmea_scanning = 2;
									}											
									break;
									
							case 2:
									if(strstr(gps.buf,"$GPGSV")!=NULL){
											if ((nmea_parse(&parser, (char*)gps.buf, (int)strlen((char*)gps.buf), &info)) > 0 )
											{																																																	
														inview_value =  info.satinfo.inview;
											}
											gps_nmea_scanning = 0;
									}											
								break;			
							
							default:
								break;
						}					
								
						memset(gps.buf,0,sizeof(char)*100);
						gps.index = 0;
						gps.gps_state = START;					
				}	
		}		
}
