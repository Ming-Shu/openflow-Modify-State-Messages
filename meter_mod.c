#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include"channel_communication.h"
#include "ofp_type_message.h"
#include "flow_mod.h"
#include "meter_mod.h"
#include "oxm_match.h"
#include "proxy_table.h"
#include "list.h"
extern struct virtual_onu_olt_port *pOnutab;

int check_instruction(char* buffer,int buf_len,enum ofp_instruction_type type)
{
 printf("------------------Staring check meter if exist!-------------\n\n");
  int value;
  uint8_t *pOxm_tlv;
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction * inst_check;

	flow_mod = (struct ofp_flow_mod *)buffer;

  	flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  	inst_check =(struct ofp_instruction *)(pOxm_tlv);
	if (type ==htons(inst_check->type))
		value =  htons(inst_check->type);
	else{ 
		inst_check =(struct ofp_instruction *)(pOxm_tlv+htons(inst_check->len));
		if (type ==htons(inst_check->type))
			value =  htons(inst_check->type);
		else
			value = -1;
	}//else
 printf("------------------Ending check meter value%d-------------\n\n",value);
	return value; 
}

int read_meter_id(struct ofp_instruction_meter *meter)
{
 int meter_id;
 	
 if(htons(meter->type)!=OFPIT_METER){
 	printf("------------------instruction is error-------------\n\n");
	meter_id =-1;
 }else{
	meter_id = htonl(meter->meter_id);
	printf("-------------meter_id : %d-------------\n\n",meter_id);	
 }//else		

  return meter_id;		
}//read_meter_id

void meter_flow_entry(char* buffer,int buf_len,int ofsw_sockfd)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions *inst_actions;
  int in_port,out_port,vlan;
  int inst_type;	
  enum proxy_entry_type	direction;
 
  inst_type = check_instruction(buffer,buf_len,OFPIT_APPLY_ACTIONS);
 
  flow_mod = (struct ofp_flow_mod*)buffer;
    	
  in_port = read_match_info(flow_mod,OFPXMT_OFB_IN_PORT);
  	printf("in_port :%d\n",in_port);

  if(inst_type == OFPIT_APPLY_ACTIONS){		  
  	 out_port = read_action_info(flow_mod,OFPAT_OUTPUT);
	 printf("out_port  :%d\n",out_port );

	 if(out_port == OFPP_CONTROLLER){
		outport_controller_entry(buffer,buf_len,ofsw_sockfd,in_port);
 	}else{
		direction = Flow_direction(in_port,out_port);	
		proxy_flowmod_direct(direction,buffer,buf_len,ofsw_sockfd,in_port,out_port);
 	}//else

  }else{
	out_port = -1;
  }//else	
     
}//meter_flow_entry


void proxy_flowmod_direct(enum proxy_entry_type direction,char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port)
{
  switch(direction) {
	case ONU_TO_OFSW:{
			printf("The flow direction is 'ONU_TO_OFSW'\n" );
			upstrem_flow_entry(buffer,buf_len,ofsw_sockfd,in_port,out_port);	
			break;
		}
	case OFSW_TO_ONU:{
			printf("The flow direction is 'OFSW_TO_ONU'\n" );
			downstrem_flow_entry(buffer,buf_len,ofsw_sockfd,in_port,out_port);			
			break;
		}
	case OFSW_TO_OFSW:{
			printf("The flow direction is 'OFSW_TO_OFSW'\n" );
			ofsw_flow_entry(buffer,buf_len,ofsw_sockfd,in_port,out_port);			
			break;
		}
	case ONU_TO_ONU:{
			printf("The flow direction is 'ONU_TO_ONU'\n" );
			onu_flow_entry(buffer,buf_len,ofsw_sockfd,in_port,out_port);			
			break;
		}
	default:{
			printf("That is error\n" );
			break;
		}
 }//switch	
}

void outport_controller_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port)
{
 printf("--------entry send to controller----------\n\n");
 int use_vlan,new_in_port,flow_length;
 struct ofp_flow_mod *flow_mod,*pflow_mod;
 struct ofp_instruction_actions *inst_actions;
 struct ofp_instruction_goto_table  *goto_table;
 struct ofp_action_header  *action_header=NULL;
 flow_mod = (struct ofp_flow_mod *)buffer;

 if(in_port>ONU_PORT_NUM){
	new_in_port=of_init_port(in_port); 
 	modify_match_info(&flow_mod,OFPXMT_OFB_IN_PORT,new_in_port);
 	send(ofsw_sockfd,flow_mod,buf_len,0);
 }else{

 	proxy_entry_convert(&in_port,NULL,&use_vlan,NULL,pOnutab);
	printf("in_port:%d,use_vlan:%d\n\n",in_port,use_vlan);

 	Proxy_FlowMod(&pflow_mod,0,flow_mod->command,flow_mod->idle_timeout,flow_mod->hard_timeout,OFPP_ANY,OFPP_ANY);	
 	Proxy_Match(&pflow_mod,OXM_OF_IN_PORT,in_port);
  	Proxy_Match(&pflow_mod,OXM_OF_VLAN_VID,use_vlan);
  	Proxy_instruction_actions(&pflow_mod,&inst_actions,OFPIT_APPLY_ACTIONS);
  	Proxy_action_header(&pflow_mod,&inst_actions,&action_header,OFPAT_POP_VLAN);
  	Proxy_instruction_goto_table(&pflow_mod,&goto_table,&action_header,use_vlan);
  	flow_length = htons(pflow_mod->header.length);
  	send(ofsw_sockfd,pflow_mod,flow_length,0);
	printf("--------Sending goto table %d----------\n\n",use_vlan);
  	free(pflow_mod);

  	flow_mod->table_id	 = use_vlan;
	modify_match_info(&flow_mod,OFPXMT_OFB_IN_PORT,in_port);		
	send(ofsw_sockfd,flow_mod,buf_len,0);
 }
}//outport_controller_entry

void onu_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port)
{
  char *creat_flow;
  struct ofp_flow_mod *flow_mod,*pflow_mod;
  struct ofp_instruction_actions *inst_actions;
  struct ofp_instruction_goto_table  *goto_table;
  struct ofp_action_header  *action_header=NULL;
  uint8_t *oxm_tlv,*pOxm_tlv;
  int flow_length,use_vlan=-1,set_vlan=-1;
  flow_mod = (struct ofp_flow_mod *)buffer;
  proxy_entry_convert(&in_port,&out_port,&use_vlan,&set_vlan,pOnutab);
  printf("in_port:%d,out_port:%d,use_vlan:%d\n\n",in_port,out_port,use_vlan);

  Proxy_FlowMod(&pflow_mod,0,flow_mod->command,flow_mod->idle_timeout,flow_mod->hard_timeout,OFPP_ANY,OFPP_ANY);	
  Proxy_Match(&pflow_mod,OXM_OF_IN_PORT,in_port);
  Proxy_Match(&pflow_mod,OXM_OF_VLAN_VID,use_vlan);
  Proxy_instruction_actions(&pflow_mod,&inst_actions,OFPIT_APPLY_ACTIONS);
  Proxy_action_header(&pflow_mod,&inst_actions,&action_header,OFPAT_POP_VLAN);
  Proxy_instruction_goto_table(&pflow_mod,&goto_table,&action_header,use_vlan);
  flow_length = htons(pflow_mod->header.length);
  send(ofsw_sockfd,pflow_mod,flow_length,0);
  free(pflow_mod);

  creat_flow =(char*)malloc(buf_len+sizeof(struct ofp_action_push)+sizeof(struct ofp_action_set_field)+sizeof(char)*8);
  memset(creat_flow, 0,buf_len);	
  memcpy(creat_flow,buffer,buf_len);
  flow_mod = (struct ofp_flow_mod *)creat_flow;

  modify_match_info(&flow_mod,OFPXMT_OFB_IN_PORT,in_port);

  flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  inst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);
  action_header =&inst_actions->actions;
  actions_type_address(&action_header,inst_actions->len,OFPAT_OUTPUT);

  inst_actions->len = inst_actions->len-action_header->len;
  flow_mod->header.length = flow_mod->header.length-action_header->len;

  Proxy_action_push(&flow_mod,&inst_actions,&action_header,OFPAT_PUSH_VLAN,0x8100);
  Proxy_action_set_field(&flow_mod,&inst_actions,&action_header,set_vlan);
  if(in_port==out_port)	
  	Proxy_action_output(&flow_mod,&inst_actions,&action_header,OFPP_IN_PORT);
  else
  	Proxy_action_output(&flow_mod,&inst_actions,&action_header,out_port);
  //printf("Send a flow_length:%d\n\n",htons(flow_mod->header.length));

  flow_length = htons(flow_mod->header.length);
  flow_mod->table_id	 = use_vlan;
  send(ofsw_sockfd,flow_mod,flow_length,0);
  free(flow_mod);
}//onu_flow_entry

void ofsw_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port)
{
 struct ofp_flow_mod *flow_mod;
 proxy_entry_convert(&in_port,&out_port,NULL,NULL,pOnutab);
 flow_mod = (struct ofp_flow_mod *)buffer;
 modify_match_info(&flow_mod,OFPXMT_OFB_IN_PORT,in_port);
 modify_action_info(&flow_mod,OFPAT_OUTPUT,out_port);//out_port
 send(ofsw_sockfd,flow_mod,buf_len,0);
}//ofsw_flow_entry

void downstrem_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port)
{
  char *creat_flow;
  struct ofp_flow_mod *flow_mod,*pflow_mod;
  struct ofp_instruction_actions *inst_actions;
  struct ofp_instruction_goto_table  *goto_table;
  struct ofp_action_header  *action_header=NULL;
  uint8_t *pOxm_tlv;
  int flow_length,set_vlan;

  creat_flow =(char*)malloc(buf_len+sizeof(struct ofp_action_push)+sizeof(struct ofp_action_set_field)+sizeof(char)*8);
  memset(creat_flow, 0,buf_len);	
  memcpy(creat_flow,buffer,buf_len);
  flow_mod = (struct ofp_flow_mod *)creat_flow;
  proxy_entry_convert(&in_port,&out_port,NULL,&set_vlan,pOnutab);
  printf("in_port:%d,out_port:%d,set_vlan:%d\n\n",in_port,out_port,set_vlan);

  modify_match_info(&flow_mod,OFPXMT_OFB_IN_PORT,in_port);

  flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  inst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);
  action_header =&inst_actions->actions;
  actions_type_address(&action_header,inst_actions->len,OFPAT_OUTPUT);

  inst_actions->len = inst_actions->len-action_header->len;
  flow_mod->header.length = flow_mod->header.length-action_header->len;

  Proxy_action_push(&flow_mod,&inst_actions,&action_header,OFPAT_PUSH_VLAN,0x8100);
  Proxy_action_set_field(&flow_mod,&inst_actions,&action_header,set_vlan);
  Proxy_action_output(&flow_mod,&inst_actions,&action_header,out_port);
  //printf("Send a flow_length:%d\n\n",htons(flow_mod->header.length));

  flow_length = htons(flow_mod->header.length);
  send(ofsw_sockfd,flow_mod,flow_length,0);
  free(flow_mod);
}

void upstrem_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port)
{
  struct ofp_flow_mod *flow_mod,*pflow_mod;
  struct ofp_instruction_actions *inst_actions;
  struct ofp_instruction_goto_table  *goto_table;
  struct ofp_action_header  *action_header=NULL;
  uint8_t *oxm_tlv,*pOxm_tlv;
  int flow_length,use_vlan=-1;

  pflow_mod = (struct ofp_flow_mod *)buffer;

  proxy_entry_convert(&in_port,&out_port,&use_vlan,NULL,pOnutab);
  printf("in_port:%d,out_port:%d,use_vlan:%d\n\n",in_port,out_port,use_vlan);

  Proxy_FlowMod(&flow_mod,0,pflow_mod->command,pflow_mod->idle_timeout,pflow_mod->hard_timeout,OFPP_ANY,OFPP_ANY);	
  Proxy_Match(&flow_mod,OXM_OF_IN_PORT,in_port);
  Proxy_Match(&flow_mod,OXM_OF_VLAN_VID,use_vlan);
  Proxy_instruction_actions(&flow_mod,&inst_actions,OFPIT_APPLY_ACTIONS);
  Proxy_action_header(&flow_mod,&inst_actions,&action_header,OFPAT_POP_VLAN);
  Proxy_instruction_goto_table(&flow_mod,&goto_table,&action_header,use_vlan);
  flow_length = htons(flow_mod->header.length);
  send(ofsw_sockfd,flow_mod,flow_length,0);
  free(flow_mod);


  pflow_mod->table_id	 = use_vlan;
  modify_match_info(&pflow_mod,OFPXMT_OFB_IN_PORT,in_port);
  /*if(in_port==out_port)		
  	modify_action_info(&pflow_mod,OFPAT_OUTPUT,OFPP_IN_PORT);	
  else*/
	modify_action_info(&pflow_mod,OFPAT_OUTPUT,out_port);//out_port		
  send(ofsw_sockfd,pflow_mod,buf_len,0);			
}//upstrem_flow_entry



void modify_action_info(struct ofp_flow_mod **pflow_mod,enum ofp_action_type type,int value)
{
  uint8_t *pOxm_tlv;
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions* inst_actions;
  struct ofp_action_header *action;
  
	flow_mod = *pflow_mod;
  	flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  	inst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);
	action =&inst_actions->actions;
	actions_type_address(&action,inst_actions->len,type);
	switch(type){
		case OFPAT_OUTPUT:{
				printf("Action header that is 'OFPAT_OUTPUT'\n\n" );
				struct ofp_action_output *p;
				p = action;
				p->port = htonl(value);
				break;
		}
		case OFPAT_SET_FIELD:{
				printf("Action header that is 'OFPAT_SET_FIELD'\n\n" );
				break;
		}
		default:{
			printf("Action header that is not exist!\n" );
			break;
		}	
	}//switch
	
}//modify_action_info



int read_action_info(struct ofp_flow_mod *flow_mod,enum ofp_action_type type)
{
  int value;
  uint8_t *pOxm_tlv;
  struct ofp_instruction_actions* inst_actions;
  struct ofp_action_header *action;

  	flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  	inst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);
	action =&inst_actions->actions;
	actions_type_address(&action,inst_actions->len,type);
	switch(type){
		case OFPAT_OUTPUT:{
				printf("Action header that is 'OFPAT_OUTPUT'\n\n" );
				struct ofp_action_output *p;
				p = action;
				value = htonl(p->port);
				break;
		}
		case OFPAT_PUSH_VLAN:{
				printf("Action header that is 'OFPAT_PUSH_VLAN'\n\n" );
				break;
		}
		case OFPAT_POP_VLAN:{
				printf("Action header that is 'OFPAT_POP_VLAN'\n\n" );
				break;
		}
		case OFPAT_SET_FIELD:{
				printf("Action header that is 'OFPAT_SET_FIELD'\n\n" );
				break;
		}
		default:{
			printf("Action header that is not exist!\n" );
			break;
		}	
	}//switch

  return value;		
}//read_action_info








