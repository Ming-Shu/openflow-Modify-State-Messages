#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include"channel_communication.h"
#include "ofp_type_message.h"
#include "flow_mod.h"
#include "oxm_match.h"
#include "proxy_table.h"
#include "list.h"
#include "olt_traffic_manage.h"
extern struct virtual_onu_olt_port *pOnutab;

int cntl_fd;

int check_instruction(char* buffer,int buf_len,enum ofp_instruction_type type)
{
 printf("------------------Staring check meter if exist!-------------\n\n");
  int value;
  uint8_t *pOxm_tlv;
  char *cOxm_tlv;	
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction * inst_check;

	flow_mod = (struct ofp_flow_mod *)buffer;

  	flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  	inst_check =(struct ofp_instruction *)(pOxm_tlv);
	cOxm_tlv = (char*)pOxm_tlv;
	if (type ==htons(inst_check->type))
		value =  htons(inst_check->type);
	else{ 
		inst_check =(struct ofp_instruction *)(cOxm_tlv+htons(inst_check->len));
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

int flow_mod_handle(char* buffer,int buf_len,int cntl_sockfd,int ofsw_sockfd)
{
 printf("------------------Staring handle 'flow_mod' message from controller-------------\n\n");
 int len_c=0;
 char* pbuffer;
 pbuffer = buffer;
 struct ofp_flow_mod *flow_mod;
 cntl_fd =  cntl_sockfd;
 while(len_c<buf_len)
 {           
             flow_mod = (struct ofp_flow_mod*)pbuffer;     
             len_c+=htons(flow_mod->header.length);
     	     printf("flow_mod_handle buf_len :%d\n,len_c:%d\n,header.length:%d\n\n",buf_len,len_c,htons(flow_mod->header.length));


  switch(flow_mod->command) {
	case OFPFC_ADD:{
			printf("flow_mod command---->OFPFC_ADD\n" );			
			//modfiy_flow_entry(buffer,buf_len,ofsw_sockfd);
			add_flow_entry(pbuffer,htons(flow_mod->header.length),ofsw_sockfd);
			printf("Sending a 'ofp_flow_mod' message to switch....\n\n");

			#ifndef	SEND
			send(ofsw_sockfd,buffer,buf_len,0);
			#endif
			break;
		}
	case OFPFC_MODIFY:{
			printf("flow_mod command---->OFPFC_MODIFY\n" );			
			break;
		}
	case OFPFC_MODIFY_STRICT:{
			printf("flow_mod command---->OFPFC_MODIFY_STRICT\n" );
            flow_entry(pbuffer,htons(flow_mod->header.length),ofsw_sockfd);			
			break;
		}
	case OFPFC_DELETE:{
			printf("flow_mod command---->OFPFC_DELETE\n" );
            flow_entry(pbuffer,htons(flow_mod->header.length),ofsw_sockfd);			
			break;
		}
	case OFPFC_DELETE_STRICT:{
			printf("flow_mod command---->OFPFC_DELETE_STRICT\n" );			
			break;
		}
	default:{
			printf("flow_mod command is error\n" );
			break;
		}
  }//switch
	
  printf("----------------------Ending handle'flow_mod' message -----------------------\n\n");
  pbuffer +=htons(flow_mod->header.length);
 }//while      
 return 0;                  
}//flow_mod_handle

/*
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
	printf("------------meter_flow_entry not output!-----------------------\n\n");
  }//else	
     
}//meter_flow_entry
*/
void flow_entry(char* buffer,int buf_len,int ofsw_sockfd)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions *inst_actions;
  int in_port,out_port,vlan;
  enum proxy_entry_type	direction;
  
  flow_mod = (struct ofp_flow_mod*)buffer;
 
   	
  flow_mod = (struct ofp_flow_mod *)buffer;	
  in_port = read_match_info(flow_mod,OFPXMT_OFB_IN_PORT);
  	printf("in_port :%d\n",in_port);		  
  out_port = read_action_info(flow_mod,OFPAT_OUTPUT);
	 printf("out_port  :%d\n",out_port );

  vlan = read_match_info(flow_mod,OFPXMT_OFB_VLAN_VID);
  	printf("vlan :%d\n",vlan);

 if(in_port>0){

   if(flow_mod->command==OFPFC_MODIFY)	
      printf("----------------------modify flow_entry -----------------------\n\n");

   if(flow_mod->command==OFPFC_MODIFY_STRICT){   
      printf("----------------------modify_strict flow_entry -----------------------\n\n");
      remove_match_in_table(buffer,buf_len);

   if(OFPIT_APPLY_ACTIONS == check_instruction(buffer,buf_len,OFPIT_APPLY_ACTIONS))
      copy_match_to_table(buffer,buf_len,out_port);
   }

   if(flow_mod->command==OFPFC_DELETE){   
      printf("----------------------delete flow_entry -----------------------\n\n");
      all_remove_entry();
   }
      
   if(flow_mod->command==OFPFC_DELETE_STRICT){   
      printf("----------------------delete_strict flow_entry -----------------------\n\n");
      remove_match_in_table(buffer,buf_len);
   }

   if(out_port == OFPP_CONTROLLER){
	outport_controller_entry(buffer,buf_len,ofsw_sockfd,in_port);
   }else{
  	direction = Flow_direction(in_port,out_port);	
  	proxy_flowmod_direct(direction,buffer,buf_len,ofsw_sockfd,in_port,out_port);
   }//else 
 }//if(in_port>0)    
}//del_flow_entry

void add_flow_entry(char* buffer,int buf_len,int ofsw_sockfd)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions *inst_actions;
  int in_port,out_port,vlan;
  enum proxy_entry_type	direction;	
  printf("----------------------add a flow_entry -----------------------\n\n");	
  flow_mod = (struct ofp_flow_mod *)buffer;	
  in_port = read_match_info(flow_mod,OFPXMT_OFB_IN_PORT);
  	printf("in_port :%d\n",in_port);		  
  out_port = read_action_info(flow_mod,OFPAT_OUTPUT);
	 printf("out_port  :%d\n",out_port );

  vlan = read_match_info(flow_mod,OFPXMT_OFB_VLAN_VID);
  	printf("vlan :%d\n",vlan);

 if(in_port>0){

   if(out_port == OFPP_CONTROLLER){
 	outport_controller_entry(buffer,buf_len,ofsw_sockfd,in_port);
   }else{
  	direction = Flow_direction(in_port,out_port);
  	
  	if(direction ==ONU_TO_OFSW||direction ==ONU_TO_ONU||direction == OFSW_TO_ONU){
	   if(OFPIT_APPLY_ACTIONS == check_instruction(buffer,buf_len,OFPIT_APPLY_ACTIONS))
   	     copy_match_to_table(buffer,buf_len,out_port);//save match and outport relative data
         }//if      
  	proxy_flowmod_direct(direction,buffer,buf_len,ofsw_sockfd,in_port,out_port);
   }//else
 }// if(in_port>0)	
}// add_flow_entry

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
  char *creat_flow,*cOxm_tlv;
  char rate_name[30];
  struct ofp_flow_mod *flow_mod,*pflow_mod;
  struct ofp_instruction_actions *inst_actions;
  struct ofp_instruction_goto_table  *goto_table;
  struct ofp_action_header  *action_header=NULL;
  struct ofp_instruction * inst_check;
	
  uint8_t *oxm_tlv,*pOxm_tlv;
  int flow_length,use_vlan=-1,set_vlan=-1,meter_id,rate;

  int pon_Id=-1,onu_Id=-1,port_Id=-1;

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
//Meter
  inst_check = (struct ofp_instruction *)(pOxm_tlv);
  cOxm_tlv = (char*)pOxm_tlv;
  if (htons(inst_check->type)==OFPIT_METER)
	    inst_actions =(struct ofp_instruction_actions*)(cOxm_tlv+htons(inst_check->len));
  else
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

  if (htons(inst_check->type)==OFPIT_METER){
	    meter_id = read_meter_id(inst_check);
	    rate = cache_meter_rate(meter_id);
	    search_onu_info(in_port,use_vlan,pOnutab,&pon_Id,&onu_Id,&port_Id);
	    bandwidth_adj(pon_Id,onu_Id,port_Id,use_vlan,rate,cntl_fd);
  }//if OFPIT_METER
	

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
  char *creat_flow,*cOxm_tlv;
  char rate_name[30];	
  struct ofp_flow_mod *flow_mod,*pflow_mod;
  struct ofp_instruction_actions *inst_actions;
  struct ofp_instruction_goto_table  *goto_table;
  struct ofp_action_header  *action_header=NULL;
  struct ofp_instruction * inst_check;

  uint8_t *pOxm_tlv;
  int flow_length,set_vlan,meter_id,rate;

  creat_flow =(char*)malloc(buf_len+sizeof(struct ofp_action_push)+sizeof(struct ofp_action_set_field)+sizeof(char)*8);
  memset(creat_flow, 0,buf_len);	
  memcpy(creat_flow,buffer,buf_len);
  flow_mod = (struct ofp_flow_mod *)creat_flow;
  proxy_entry_convert(&in_port,&out_port,NULL,&set_vlan,pOnutab);
  printf("in_port:%d,out_port:%d,set_vlan:%d\n\n",in_port,out_port,set_vlan);

  modify_match_info(&flow_mod,OFPXMT_OFB_IN_PORT,in_port);

  flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
//Meter
  inst_check = (struct ofp_instruction *)(pOxm_tlv);
  cOxm_tlv = (char*)pOxm_tlv;
  if (htons(inst_check->type)==OFPIT_METER){
	    /*meter_id = read_meter_id(inst_check);
	    cache_meter_rate(meter_id);
	    rate = cache_meter_rate(meter_id);
	    sprintf(rate_name, "VLAN_S%d",set_vlan);
	    u_sla_add(&rate_name,rate,rate*1.2,0,0);*/
	    inst_actions =(struct ofp_instruction_actions*)(cOxm_tlv+htons(inst_check->len));
  }else
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
  char rate_name[30];
  struct ofp_instruction * inst_check;

  struct ofp_flow_mod *flow_mod,*pflow_mod;
  struct ofp_instruction_actions *inst_actions;
  struct ofp_instruction_goto_table  *goto_table;
  struct ofp_action_header  *action_header=NULL;
  uint8_t *oxm_tlv,*pOxm_tlv;
  int flow_length,use_vlan=-1,meter_id,rate;

  int pon_Id=-1,onu_Id=-1,port_Id=-1;

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

//Meter
  if(OFPIT_METER == check_instruction(buffer,buf_len,OFPIT_METER))
  {
	flow_mod_offset(&pOxm_tlv, pflow_mod->match.oxm_fields,htons(pflow_mod->match.length));
  	inst_check = (struct ofp_instruction *)(pOxm_tlv);
  }//if

  pflow_mod->table_id	 = use_vlan;
  modify_match_info(&pflow_mod,OFPXMT_OFB_IN_PORT,in_port);
  /*if(in_port==out_port)		
  	modify_action_info(&pflow_mod,OFPAT_OUTPUT,OFPP_IN_PORT);	
  else*/
	modify_action_info(&pflow_mod,OFPAT_OUTPUT,out_port);//out_port		
  send(ofsw_sockfd,pflow_mod,buf_len,0);

//Meter BW adj
 if (OFPIT_METER == check_instruction(buffer,buf_len,OFPIT_METER)){
	    meter_id = read_meter_id(inst_check);
	    rate = cache_meter_rate(meter_id);
	    search_onu_info(in_port,use_vlan,pOnutab,&pon_Id,&onu_Id,&port_Id);
	    bandwidth_adj(pon_Id,onu_Id,port_Id,use_vlan,rate,cntl_fd);
  }//if OFPIT_METER
			
}//upstrem_flow_entry


void modify_match_info(struct ofp_flow_mod **pflow_mod,enum oxm_ofb_match_fields field,int value)
{
  struct ofp_flow_mod *flow_mod;
  int match_len;
  uint32_t *w_oxm_tlv,oxm_type;
  uint16_t *w_2byte_oxm_tlv; 
  uint8_t *pOxm_tlv;

	flow_mod = *pflow_mod;
	pOxm_tlv = &flow_mod->match.oxm_fields;
	match_len=htons(flow_mod->match.length)-4;//The size of type & len is 4 byte 
	while(OXM_FIELD(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))!=field && match_len>0)
	{
		match_len-=(OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);	
		pOxm_tlv+=(OXM_LENGTH(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);
    
	}

	  switch(field) {
		case OFPXMT_OFB_IN_PORT:{
				oxm_type = OXM_OF_IN_PORT;
				printf("case OFPXMT_OFB_IN_PORT\n" );
			 break; 
		}
		case OFPXMT_OFB_VLAN_VID:{
 				oxm_type = OXM_OF_VLAN_VID;
				printf("case  OFPXMT_OFB_VLAN_VID\n" );
			 break; 
		}
		default:{
			printf("The oxm_type is not exist\n" );
			break;
		}
	  }//switch
     
	if(OXM_FIELD(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))==field)
	{
          
	  switch(oxm_type) {
		case OXM_OF_IN_PORT:{
			 w_oxm_tlv = pOxm_tlv;
			*w_oxm_tlv =  ntohl(oxm_type);
			*(w_oxm_tlv+1)=ntohl(value);
			printf("value:%d\n\n",value);
			 break; 
		}
		case OXM_OF_VLAN_VID:{
 			 w_oxm_tlv = pOxm_tlv;
 			*w_oxm_tlv =  ntohl(oxm_type);
  			 w_2byte_oxm_tlv= (w_oxm_tlv+1);
 			*(w_2byte_oxm_tlv)=ntohs(value);
			printf("value:%d\n\n",value);
			 break; 
		}
		default:{
			printf("The oxm_type is not exist\n" );
			break;
		}
	  }//switch
	}//if  
		
}// modify_match_info

void modify_action_info(struct ofp_flow_mod **pflow_mod,enum ofp_action_type type,int value)
{
  uint8_t *pOxm_tlv;
  char* cOxm_tlv;
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction * inst_check;
  struct ofp_instruction_actions* inst_actions;
  struct ofp_action_header *action;
  int meter_id;  

	flow_mod = *pflow_mod;
  	flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
	inst_check = (struct ofp_instruction *)(pOxm_tlv);

	cOxm_tlv = (char*)pOxm_tlv;
	if (htons(inst_check->type)!=OFPIT_APPLY_ACTIONS){
	    meter_id = read_meter_id(inst_check);
	    cache_meter_rate(meter_id);
	    inst_actions =(struct ofp_instruction_actions*)(cOxm_tlv+htons(inst_check->len));
	}else
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

void Proxy_action_output(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,int value)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions  *instruction; 
  struct ofp_action_output *output;

  flow_mod =* pflow_mod;
  instruction =* instruction_actions;

  if(*action==NULL)
  	output = &instruction->actions;
  else
	output =*action;	

  output->type = htons(OFPAT_OUTPUT);     
  output->len = htons(sizeof(struct ofp_action_output));       
  output->port =htonl(value);
  output->max_len =htons(0);

  instruction->len  = instruction->len+output->len;
  flow_mod->header.length = flow_mod->header.length+output->len;

}//Proxy_action_output

void Proxy_action_set_field(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,int value)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions  *instruction;
  struct ofp_action_set_field *set_field;
  uint32_t *w_oxm_action_field;
  uint16_t *w_2byte_oxm_tlv;
  uint8_t  oxm_len,*pOxm_tlv,*least_oxm_address;
  
  flow_mod =* pflow_mod;
  instruction =* instruction_actions;

  if(*action==NULL)
  	set_field = &instruction->actions;
  else
  	set_field =*action;

  set_field->type = htons(OFPAT_SET_FIELD); 
  w_oxm_action_field = pOxm_tlv = &set_field->field;
  *w_oxm_action_field =  ntohl(OXM_OF_VLAN_VID);
  w_2byte_oxm_tlv= (w_oxm_action_field+1);//w_oxm_action_field+1 =oxm_tlv_header memory 4byte
  *(w_2byte_oxm_tlv)=ntohs(value); 
  least_oxm_address=w_2byte_oxm_tlv+1;
  oxm_len = OXM_LENGTH(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)));
  set_field->len = htons(sizeof(struct ofp_action_set_field))+htons(OFP_ACTION_SET_FIELD_OXM_PADDING(oxm_len+4))+htons(oxm_len);//oxm_tlv_field 4byte
  
  instruction->len  = instruction->len+set_field->len;
  flow_mod->header.length = flow_mod->header.length+set_field->len;
/*
printf("Proxy_set_field->len%d\n\n",htons(set_field->len));
printf("Proxy_instruction->len:%d\n\n",htons(instruction->len));
printf("Proxy_action_header_length:%d\n\n",htons(flow_mod->header.length));*/

  *action= least_oxm_address+OFP_ACTION_SET_FIELD_OXM_PADDING(oxm_len+4);//next pointer address
}//Proxy_action_set_field

void Proxy_action_push(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,enum ofp_action_type type, uint16_t ethertype)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_action_push *push;
  struct ofp_instruction_actions  *instruction;
  
  flow_mod =* pflow_mod;
  instruction =* instruction_actions;
  
  if(*action==NULL)
  	push = &instruction->actions;
  else
	push =*action;

  push->type = htons(type);     
  push->len = htons(sizeof(struct ofp_action_push));       
  push->ethertype = htons(ethertype); 
  instruction->len  = instruction->len+push->len;
  flow_mod->header.length = flow_mod->header.length+push->len;
/*
printf("Proxy_instruction_push->len:%d\n\n",htons(instruction->len));
printf("Proxy_action_header_length:%d\n\n",htons(flow_mod->header.length));
*/
  *action= push+1 ;//next pointer address
}//Proxy_action_push

void Proxy_action_header(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,enum ofp_action_type type)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions  *instruction;
  struct ofp_action_header *action_header;
  
  flow_mod =* pflow_mod;
  instruction =* instruction_actions;

  if(*action==NULL)
  	action_header = &instruction->actions;
  else
	action_header =*action;	

  action_header->type = htons(type);     
  action_header->len = htons(sizeof(struct ofp_action_header));       

  instruction->len  = instruction->len+action_header->len;
  flow_mod->header.length = flow_mod->header.length+action_header->len;
/*
printf("Proxy_action_header->len:%d\n\n",htons(action_header->len));
printf("Proxy_instruction->len:%d\n\n",htons(instruction->len));
printf("Proxy_action_header_length:%d\n\n",htons(flow_mod->header.length));*/

  *action= action_header+1;//next pointer	
}//Proxy_action_header

void Proxy_instruction_goto_table(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_goto_table  **instruction_goto_table,struct ofp_action_header **action,uint8_t table_id)
{
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_goto_table *goto_table;

  flow_mod=*pflow_mod;
  
  goto_table = *action;		
  goto_table->type = htons(OFPIT_GOTO_TABLE);
  goto_table->len  = htons(sizeof(struct ofp_instruction_goto_table));
  goto_table->table_id = table_id;

  *instruction_goto_table = goto_table;
  flow_mod->header.length = flow_mod->header.length+goto_table->len;

}//Proxy_instruction_goto_table

void Proxy_instruction_actions(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,enum ofp_instruction_type type)
{
  uint8_t *pOxm_tlv;
  struct ofp_flow_mod *flow_mod;
  struct ofp_instruction_actions *instruction;

  flow_mod=*pflow_mod;

  flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  instruction =(struct ofp_instruction_actions*)(pOxm_tlv);
  instruction->type = htons(type);
  instruction->len  = htons(sizeof(struct ofp_instruction_actions));

  flow_mod->header.length = flow_mod->header.length+instruction->len;

  *instruction_actions =instruction;	
	
}//Proxy_instruction_actions

void Proxy_Match(struct ofp_flow_mod **pflow_mod,uint32_t oxm_type,int value)
{
  struct ofp_flow_mod *flow_mod;	
  uint32_t *address;
  uint32_t *w_oxm_tlv;
  uint16_t *w_2byte_oxm_tlv; 
  uint16_t  match_len;

  flow_mod=*pflow_mod;

  if(flow_mod->match.length==htons(sizeof(struct ofp_match))){
 	 flow_mod->match.length = (*pflow_mod)->match.length-htons(4); //match.oxm_fields is 4 byte 
	 address = &flow_mod->match.oxm_fields;
  }else{
	address = least_oxm_address(&flow_mod->match.oxm_fields,htons(flow_mod->match.length));
  }		
  switch(oxm_type) {
		case OXM_OF_IN_PORT:{
			 w_oxm_tlv = address;
			*w_oxm_tlv =  ntohl(oxm_type);
			*(w_oxm_tlv+1)=ntohl(value);
			 match_len = htons(4+4); //match.oxm_fields is 4byte ,and value is 4byte
			 break; 
		}
		case OXM_OF_VLAN_VID:{
 			 w_oxm_tlv = address;
 			*w_oxm_tlv =  ntohl(oxm_type);
  			 w_2byte_oxm_tlv= (w_oxm_tlv+1);
 			*(w_2byte_oxm_tlv)=ntohs(value);
			 match_len = htons(4+2);//match.oxm_fields is 4byte ,and value is 2byte
			 break; 
		}
		default:{
			printf("The oxm_type is not exist\n" );
			break;
		}
  }//switch
flow_mod->match.length =  flow_mod->match.length+match_len; 

flow_mod->header.length = htons(sizeof(struct ofp_flow_mod))-htons(sizeof(struct ofp_match))+flow_mod->match.length+htons(OFP_MATCH_OXM_PADDING(htons(flow_mod->match.length)));

}//proxy_Match

void Proxy_FlowMod(struct ofp_flow_mod **pflow_mod,uint8_t table_id,enum ofp_flow_mod_command cmd,uint16_t idle_timeout,uint16_t hard_timeout,uint32_t out_port,uint32_t out_group)
{
  char *creat_flow;
  int length = FLOWMODMAX;
  struct ofp_flow_mod *flow_mod;

  creat_flow =(char*)malloc(length);
  memset(creat_flow, 0,length);

  *pflow_mod = (struct ofp_flow_mod *)creat_flow;
  flow_mod = *pflow_mod;			
  flow_mod->header.version = OFP_VERSION;
  flow_mod->header.type	  = OFPT_FLOW_MOD;
  flow_mod->header.length = htons(sizeof(struct ofp_flow_mod));
  flow_mod->header.xid	 = htonl(11);
  flow_mod->cookie	 = 0x00ULL;
  flow_mod->cookie_mask	 = 0x00ULL;
  flow_mod->table_id	 = table_id;
  flow_mod->command	 = cmd;
  flow_mod->idle_timeout = idle_timeout;//OFP_FLOW_PERMANENT;
  flow_mod->hard_timeout = hard_timeout;//OFP_FLOW_PERMANENT;
  flow_mod->priority	 = 0;
  flow_mod->buffer_id	 = 0;
  flow_mod->out_port	 = out_port;
  flow_mod->out_group	 = out_group;
  flow_mod->flags	 = htons(OFPFF_SEND_FLOW_REM);
  flow_mod->match.type	 = htons(OFPMT_OXM);
  flow_mod->match.length = htons(sizeof(struct ofp_match));

}//proxy_FlowMod()



int read_match_info(struct ofp_flow_mod *flow_mod,enum oxm_ofb_match_fields field)
{
  int value,match_len;
  uint8_t *pOxm_tlv;
	
	pOxm_tlv = flow_mod->match.oxm_fields;
	match_len=htons(flow_mod->match.length)-4;//The size of type & len is 4 byte 
	while(OXM_FIELD(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))!=field && match_len>0)
	{
		match_len-=(OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);	
		pOxm_tlv+=(OXM_LENGTH(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);
	}

	if(OXM_FIELD(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))==field)
		value = read_payload(pOxm_tlv+4,(OXM_LENGTH(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))));
	else
		value =-1;
  return value;	
}//read_match_info


int read_action_info(struct ofp_flow_mod *flow_mod,enum ofp_action_type type)
{
  int value;
  uint8_t *pOxm_tlv;
  char *cOxm_tlv;	
  struct ofp_instruction * inst_check;
  struct ofp_instruction_actions* inst_actions;
  struct ofp_action_header *action;

  	flow_mod_offset(&pOxm_tlv, flow_mod->match.oxm_fields,htons(flow_mod->match.length));
	inst_check = (struct ofp_instruction *)(pOxm_tlv);
	cOxm_tlv = (char*)pOxm_tlv;

	if (htons(inst_check->type)!=OFPIT_APPLY_ACTIONS){
	    inst_actions =(struct ofp_instruction_actions*)(cOxm_tlv+htons(inst_check->len));
		printf("\n\n---meter_len:%d,action_len:%d,---\n\n",htons(inst_check->len),htons(inst_actions->len));
	}else
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

void actions_type_address(struct ofp_action_header **action_header,uint16_t len,enum ofp_action_type type)
{
printf("----------------------actions_type_address -----------------------\n\n");
	struct ofp_action_header *action;
	action = *action_header;
	while(len>0&&(htons(action->type)!=type))
	{
	  len=len-(action->len);	
	  action=action+(htons(action->len)/8);
	}
	*action_header=action;	
}

enum proxy_entry_type Flow_direction(int in_port,int out_port)
{
  int type;		
	
	if(in_port<=ONU_PORT_NUM && out_port>ONU_PORT_NUM)
		type=ONU_TO_OFSW;
	else if(in_port>ONU_PORT_NUM && out_port<=ONU_PORT_NUM)
		type=OFSW_TO_ONU;
	else if(in_port>ONU_PORT_NUM && out_port>ONU_PORT_NUM)
		type=OFSW_TO_OFSW;
	else if(in_port<=ONU_PORT_NUM && out_port<=ONU_PORT_NUM)
		type=ONU_TO_ONU;
	else
		type=SYSTEM_ERROR;
	return type;
}

void modfiy_flow_entry(char* buffer,int buf_len,int ofsw_sockfd)
{
 
 printf("----------------------Modify a flow_entry -----------------------\n\n");
 
 char *creat_flow;
 int a;
 struct ofp_flow_mod *flow_mod,*pflow_mod;
 struct ofp_match* pMatch;
 struct ofp_instruction_actions *inst_actions,*pinst_actions;
 uint8_t *oxm_tlv,*pOxm_tlv;
 uint16_t *w_2byte_oxm_tlv;
 uint32_t *w_oxm_tlv; 
	
 creat_flow =(char*)malloc(buf_len+sizeof(char)*6+sizeof(struct ofp_action_set_field)+sizeof(char)*8);
 memset(creat_flow, 0,buf_len);	
#ifndef	NO_COPY
#ifdef	TEST	
 memcpy(creat_flow,buffer,sizeof(struct ofp_flow_mod)+sizeof(char)*4);
#else
 memcpy(creat_flow,buffer,buf_len);
#endif 
#endif //NO_COPY
 pflow_mod = (struct ofp_flow_mod *)buffer;
 flow_mod = (struct ofp_flow_mod *)creat_flow;	
 pMatch = &flow_mod->match;

//---------------------------------------------------------------------------
#ifdef	NO_COPY
	flow_mod->header.version = OFP_VERSION;
	flow_mod->header.type	 = OFPT_FLOW_MOD;
	flow_mod->header.length	 = pflow_mod->header.length;
	flow_mod->header.xid	 = pflow_mod->header.xid;
	flow_mod->cookie	 = pflow_mod->cookie;
	flow_mod->cookie_mask	 = pflow_mod->cookie_mask;
	flow_mod->table_id	 = pflow_mod->table_id;
	flow_mod->command	 = pflow_mod->command;
	flow_mod->idle_timeout	 = pflow_mod->idle_timeout;
	flow_mod->hard_timeout	 = pflow_mod->hard_timeout;
	flow_mod->priority	 = pflow_mod->priority;
	flow_mod->buffer_id	 = pflow_mod->buffer_id;
	flow_mod->out_port	 = pflow_mod->out_port;
	flow_mod->out_group	 = pflow_mod->out_group;
	flow_mod->flags		 = pflow_mod->flags;
	pMatch->type		 = pflow_mod->match.type;
	pMatch->length		 = pflow_mod->match.length;
#endif 
//----------------------------------------------------------------------------
#ifdef	TEST


 //pMatch->length		 =pflow_mod->match.length+htons(6);
 w_oxm_tlv = least_oxm_address(pMatch->oxm_fields,htons(pMatch->length));


 *w_oxm_tlv =  	ntohl(OXM_OF_VLAN_VID);
  oxm_tlv = (uint8_t *)(w_oxm_tlv+1);
  printf("oxm_tlv:%d\n\n",oxm_tlv);
 *(oxm_tlv+1)=  7;
 int t;
 for(t=1;t<7;t++)
	*(oxm_tlv+1+t)=  0;

#endif //TEST

//----------------------------------------------------------------------------//assign payload value
#ifdef	NO_COPY
struct ofp_action_output *action_output,*paction_output;
int lenght;

pMatch->length		 = pflow_mod->match.length+htons(6);

w_oxm_tlv = &pMatch->oxm_fields;
*w_oxm_tlv =  	ntohl(OXM_OF_IN_PORT);
*(w_oxm_tlv+1)=ntohl(6);

*(w_oxm_tlv+2)= ntohl(OXM_OF_VLAN_VID);
 w_2byte_oxm_tlv= (w_oxm_tlv+2+1);
*(w_2byte_oxm_tlv)=ntohs(7);

 flow_mod_offset(&pOxm_tlv, pflow_mod->match.oxm_fields,htons(pflow_mod->match.length));
 pinst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);

 flow_mod_offset(&pOxm_tlv, pMatch->oxm_fields,htons(pMatch->length));
 inst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);

 inst_actions->type = pinst_actions->type;
 inst_actions->len = pinst_actions->len;
 inst_actions->actions->type = pinst_actions->actions->type;
 inst_actions->actions->len = pinst_actions->actions->len;

 paction_output = pinst_actions->actions;
 action_output = inst_actions->actions;

 action_output->port = paction_output->port;
 action_output->max_len = paction_output->max_len;
 strcpy(action_output->pad,paction_output->pad);		
#endif 

#ifdef	ACTION_TEST
//----------------------------------------- instruction_actions-----------------------------------------
 struct ofp_action_output *temp;
 struct ofp_action_set_field *p_set_field;
 uint8_t*pOxm_action_field,aOxm_action_field;
 uint32_t *w_oxm_action_field;

 temp = &inst_actions->actions;
 p_set_field=temp+1;// add a 'ofp_instruction_actions' memory


 p_set_field->type = htons(OFPAT_SET_FIELD);
 p_set_field->len  = htons(16);
 w_oxm_action_field = p_set_field->field;

 *w_oxm_action_field =  ntohl(OXM_OF_VLAN_VID);
  aOxm_action_field = (uint8_t *)(w_oxm_action_field+1); //w_oxm_action_field+1 =oxm_tlv memory 4byte
 //*(aOxm_action_field+1)= 3;
  p_set_field->field[4] = 48;//?	
  p_set_field->field[5] = 4;



pOxm_action_field = p_set_field->field;
w_oxm_tlv = least_oxm_address(p_set_field->field,htons(p_set_field->len));

#ifdef	ADDRESS
printf("ofp_action_output *temp adress: %d\n",temp);
printf("p_set_field: %d\n",p_set_field)	;
for(a=0;a<40;a++)
	printf("inst_actions->actions->pad[%d],Address:%d,value:%d\n",a,&inst_actions->actions->pad[a],inst_actions->actions->pad[a]);
#endif

#endif	//ACTION_TEST
//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------	

#ifdef	TEST
 flow_mod_offset(&pOxm_tlv, pflow_mod->match.oxm_fields,htons(pflow_mod->match.length));
 pinst_actions =(struct ofp_instruction_actions*)(pOxm_tlv);
 memcpy(inst_actions,pinst_actions,sizeof(struct ofp_instruction_actions)+sizeof(struct ofp_action_header));
#endif
#ifdef	ACTION_TEST
 inst_actions->len = pinst_actions->len+p_set_field->len;
#endif

lenght= sizeof(struct ofp_flow_mod)-sizeof(struct ofp_match)+htons(pMatch->length)+htons(inst_actions->len)+OFP_MATCH_OXM_PADDING(htons(pMatch->length));

flow_mod->header.length	 = htons(lenght);

#ifdef	ADDRESS

for(a=0;a<40;a++)
	printf("pMatch->oxm_fields[%d]:address:%d,value:%x\n",a,&pMatch->oxm_fields[a],pMatch->oxm_fields[a]);
	
#endif

 printf("flow entry out_port:%08x\n\n",flow_mod->out_port);

 printf("OFP_MATCH_OXM_PADDING:%d,htons(OFP_MATCH_OXM_PADDING(htons(pMatch->length)):%d,lenght:%d\n\n",OFP_MATCH_OXM_PADDING(htons(pMatch->length)),htons(OFP_MATCH_OXM_PADDING(htons(pMatch->length))),lenght);

 printf("version=%04x, type=%d, length=%d, xid=%d\n\n", flow_mod->header.version, flow_mod->header.type, htons(flow_mod->header.length), htonl(flow_mod->header.xid));

 printf("Flow Mod:cookie=%d,cookie_mask=%d,table_id=%d command=%d\n",htonll(flow_mod->cookie),htonll(flow_mod->cookie_mask),flow_mod->table_id, flow_mod->command);
 printf("idle_timeout=%d,hard_timeout=%d,priority=%d,buffer_id=%d,out_port:%d,,out_group=%d,flags=%d\n",htons(flow_mod->idle_timeout),htons(flow_mod->hard_timeout),htons(flow_mod->priority),htonl(flow_mod->buffer_id),htonl(flow_mod->out_port),htonl(flow_mod->out_group),htons(flow_mod->flags));

printf("Flow Match:  type=%d,length=%d\n", htons(pMatch->type),htons(pMatch->length));


printf("Flow instruction_actions: type=%d,len=%d,action_type=%d,action_len=%d\n\n",htons(inst_actions->type),htons(inst_actions->len),htons(inst_actions->actions->type),htons(inst_actions->actions->len));

#ifdef	TEST
	inst_actions->len = htons(inst_actions->len+16);
#endif

if(inst_actions->actions->type==OFPAT_OUTPUT){
	printf("Action header type that is OFPAT_OUTPUT\n\n");
	struct ofp_action_output *p;
	p = &inst_actions->actions;
	printf("Action_output: type=%d,len=%d,port=%d,max_len=%d\n\n",htons(p->type),htons(p->len),htonl(p->port),htons(p->max_len));

#ifdef	DEBUG
	int outport;	
 
	outport = flow_out_port_convert(htonl(p->port),pOnutab);
	printf("flow_out_port_convert: %d\n",outport);
	p->port=htonl(outport);
#endif	
	//p->port=htonl(8);
	printf("Modify Out port is %d\n",htonl(p->port));

#ifdef	ACTION_TEST
//-------------------------------------------printf---------------------------------------------
if(p_set_field->type==htons(OFPAT_SET_FIELD))
{
	printf("OFPAT_SET_FIELD: type=%d,len=%d\n\n",htons(p_set_field->type),htons(p_set_field->len));
	oxm_match_printf(pOxm_action_field);
}
#endif	//ACTION_TEST
//printf("----------------------Modify a flow_entry end!!!-----------------------\n\n");
#ifdef	ADDRESS	
printf("Action_output Address: type=%d,len=%d,port=%d,max_len=%d\n\n",&p->type,&p->len,&p->port,&p->max_len);
#endif
}

#ifdef	SEND
send(ofsw_sockfd,flow_mod,lenght,0);
#endif
}//modfiy_flow_entry


int flow_mod_offset(uint8_t**oxm_tlv,uint8_t*pOxm_tlv,int length)
{
	 int oxm_tvl_num = 0,match_len=length-4;	
	 while(match_len>0){
		//oxm_match_printf(pOxm_tlv);
		match_len-=(OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);
		pOxm_tlv+=(OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);
		oxm_tvl_num++;
		//printf("\noxm_tvl_num:%d\n\n",oxm_tvl_num);
	}
	*oxm_tlv=pOxm_tlv+ OFP_MATCH_OXM_PADDING(length);
	
	return oxm_tvl_num;
}

uint8_t* least_oxm_address(uint8_t*pOxm_tlv,int length)
{
	 int match_len=length-4;	
	 while(match_len>0){
		match_len-=(OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);
		pOxm_tlv+=(OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)))+4);
	}
	return pOxm_tlv;
}

uint8_t read_payload(uint8_t*pOxm_tlv,int payload_len)
{
	uint8_t value;
	switch(payload_len){
		case 8:{
			value = UNPACK_OXM_TLV_PAYLOAD_8_BYTE(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3),*(pOxm_tlv+4),*(pOxm_tlv+5),*(pOxm_tlv+6),*(pOxm_tlv+7));
			break;
		}
		case 6:{
			value = UNPACK_OXM_TLV_PAYLOAD_6_BYTE(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3),*(pOxm_tlv+4),*(pOxm_tlv+5));
			break;
		}
		case 4:{
			value = UNPACK_OXM_TLV_PAYLOAD_4_BYTE(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3));
			break;
		}
		case 2:{
			value = UNPACK_OXM_TLV_PAYLOAD_2_BYTE(*pOxm_tlv,*(pOxm_tlv+1));
			break;
		}
	}//switch
return value;
}

void oxm_match_printf(uint8_t*pOxm_tlv)
{
	printf("The is printing to oxm_match...\n\n");
	printf("CLASS=0x%x,FIELD=%d,TYPE=%04x,HASMASK=%d,LENGTH=%d,VAULE=%x\n\n",\
	OXM_CLASS(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3))),\
	OXM_FIELD(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3))),\
	OXM_TYPE( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3))),\
	OXM_HASMASK( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3))),\
	OXM_LENGTH( UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3))),\
	read_payload(pOxm_tlv+4,(OXM_LENGTH(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3))))));
	
}


