#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "meter_list.h"

struct meter_list  *meter_head = NULL; 
struct meter_list  *meter_now  = NULL; 

struct meter_list  *meter_getnode (void) 
{
 printf("struct meter_list  a getnode is created\n");	
	struct meter_list  *p;
	p = (struct meter_list *) malloc(sizeof(struct meter_list));
	if( p == NULL){
		printf("memory is not enough\n");
		exit(EXIT_FAILURE);
        }
	return(p);
}

/* Initializes 'meter_list ' as an empty list. */
void meter_list_init(struct meter_list *list)
{
 printf("pointer inital of list\n");
	list->id=0;
	list->rate=0;
	list->data = NULL;    	
	list->next = NULL;
    		
}

void assign_meter_value(struct meter_list *list,int id,int rate,char *data)
{
   	list->id= id; 
  	list->rate= rate;
	list->data = data;     
}//assign_meter_value

/* Inserts 'elem' just next. */
struct meter_list  *meter_list_insert(struct meter_list  *meter_now ,struct meter_list  *newPtr)
{
 printf("---------pointer insert of meter_list----------\n\n");
		if(meter_head == NULL){
			meter_head = meter_now =newPtr;
			meter_now->next = NULL;
		}else{
              while(meter_now!=NULL){      
                               printf("now!=NULL\n");
			                   if(meter_now->next==NULL){
				                                   meter_now->next = newPtr;
				                                   newPtr->next = NULL;	
				                                   break ;
                                 }
                    meter_now = meter_now->next;             
              }//while
		}//else
	return(meter_now);	
}//list_insert

void save_meter_data(int id,int rate,char *data)
{
 printf("\n---------------- save_meter_data--------------\n"); 
 struct meter_list  *new_p;         
 new_p= meter_getnode();
 meter_list_init(new_p);
 printf("id:%d,rate:%d\n\n",id,rate);
 assign_meter_value(new_p,id,rate,data);      
 meter_now = meter_list_insert(meter_now,new_p); 
      
}//save_match_data

void copy_meter_to_table(char* buffer,int buf_len)
{
  printf("\n----------------copy_meter_to_table--------------\n");    
  char*p;
  int meter_id,rate;  
  struct ofp_meter_mod *meter_mod;    
  struct ofp_meter_band_header *band;
  meter_mod = (struct ofp_meter_mod *)buffer;  
  band = meter_mod->bands;

  meter_id = htonl(meter_mod->meter_id);	
  rate = htonl(band->rate);

  p = (char*) malloc(sizeof(char)*(htons(meter_mod->header.length)));    
  memset(p, 0,htons(meter_mod->header.length));	
  memcpy(p,meter_mod,htons(meter_mod->header.length));
  printf("meter_mod.header_length:%d,meter_id :%d,rate:%d\n\n",htons(meter_mod->header.length),meter_id,rate);  
  save_meter_data(meter_id,rate,p);    
}//copy_meter

void remove_meter_in_table(char* buffer,int buf_len)
{
  printf("\n---------------remove_meter_in_table-------------\n");    
    
  struct ofp_meter_mod *meter_mod;    
  struct ofp_meter_band_header *band;
  meter_mod = (struct ofp_meter_mod *)buffer;  
  band = meter_mod->bands;
	
  printf("meter_mod.header_length:%d,meter_id :%d,rate:%d\n\n",htons(meter_mod->header.length),htonl(meter_mod->meter_id),htonl(band->rate));
  remove_meter_list_node(buffer);  
}//remove_meter_in_table


void all_remove_meter(void)
{
  struct meter_list *p;
 
  p = meter_head;
  while(p!=NULL){
        printf("\n----------------all_remove_meter------------\n");                
	meter_list_init(p);                
        p = p->next;             
  }//while 
}//all_remove

void print_meter_tmp_table(void)
{
  struct meter_list *p;

  p = meter_head;
  
  while(p!=NULL){
        printf("\n----------------print_meter_tmp_table--------------\n");                
        printf("meter_id :%d,rate:%d\n\n",p->id,p->rate);                 
        p = p->next;             
  }//while 
}//print_metet_tmp_table

void remove_meter_list_node(char *meter)
{
    printf("\n---------------- remove_meter_list_node--------------\n");   
    struct meter_list *del,*p,*prev;
    p = meter_head;  
    while(p!=NULL){
          if(strcmp(p->data,meter)==0)
          {
            del=p;
            prev->next = p->next;         
          } 
          prev = p;        
          p = p->next;               
    }//while             
    free(del);
}// remove_list_node


int cache_meter_rate(int meter_id)
{
     printf("\n---------------- Staring cache rate of meter...... --------------\n");
     int rate;
     struct meter_list *p;
     p = meter_head;  
	
  while(p!=NULL){                
        printf("meter_id :%d,rate:%d\n\n",p->id,p->rate);
	
	if(p->id==meter_id){
	   rate = p->rate;
	   break;
	}else{		                 
           p = p->next;
	}//else             
  }//while 
  

 return (rate*0.001);           
}//cache_meter_data
