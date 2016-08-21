#include <stdio.h>
#include <stdlib.h>
#include "openflow_1_3.h"

struct meter_list 
  {
	int id;          //meter id
	int rate;		//rate of meter
	char *data;		// pointer of any data
	struct meter_list *next;	// Next list element. 	
  };

struct meter_list  *meter_getnode (void);
void meter_list_init(struct meter_list *list);
void assign_meter_value(struct meter_list *list,int id,int rate,char *data);
struct meter_list  *meter_list_insert(struct meter_list  *meter_now ,struct meter_list  *newPtr);
void save_meter_data(int id,int rate,char *data);
void copy_meter_to_table(char* buffer,int buf_len);
void remove_meter_in_table(char* buffer,int buf_len);
void all_remove_meter(void);
void print_meter_tmp_table(void);
void remove_meter_list_node(char *meter);
int cache_meter_rate(int meter_id); 
