#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openflow_1_3.h"

#define FLOWMODMAX  sizeof(struct ofp_flow_mod)+sizeof(struct ofp_match)*6+sizeof(char)*16*6+sizeof(struct ofp_instruction_goto_table)+sizeof(struct ofp_instruction_actions)+sizeof(struct ofp_action_header)+sizeof(struct ofp_action_output)+sizeof(struct ofp_action_push)+sizeof(struct ofp_action_set_field)*6+sizeof(char)*16*6;
 
#define TEST1
#define SEND
#define ACTION_TEST
#define NO_COPY
#define ADDRESS1

enum proxy_entry_type{
	ONU_TO_OFSW,
	OFSW_TO_ONU,
	OFSW_TO_OFSW,
	ONU_TO_ONU,
	SYSTEM_ERROR
};

struct proxy_flow_mod
{
	struct ofp_flow_mod *flow_mod;
	struct ofp_instruction_goto_table  *goto_table;
	struct ofp_instruction_actions	*instruction_actions;
	struct ofp_action_set_field *action_set_field;
};

int check_instruction(char* buffer,int buf_len,enum ofp_instruction_type type);
int read_meter_id(struct ofp_instruction_meter *meter);

int flow_mod_handle(char* buffer,int buf_len,int cntl_sockfd,int ofsw_sockfd);

void add_flow_entry(char* buffer,int buf_len,int ofsw_sockfd);
void flow_entry(char* buffer,int buf_len,int ofsw_sockfd);
/*The 'modify_*_info'* functions are used at field is exist and flow_entry length no change,that modify  current field. 
 *The 'read_*_info'* functions are used at current field. 
 */
int read_match_info(struct ofp_flow_mod *flow_mod,enum oxm_ofb_match_fields field);
int read_action_info(struct ofp_flow_mod *flow_mod,enum ofp_action_type type);
void actions_type_address(struct ofp_action_header **action_header,uint16_t len,enum ofp_action_type type);
void modify_action_info(struct ofp_flow_mod **pflow_mod,enum ofp_action_type type,int value);
void modify_match_info(struct ofp_flow_mod **pflow_mod,enum oxm_ofb_match_fields field,int value);

enum proxy_entry_type Flow_direction(int in_port,int out_port);
void proxy_flowmod_direct(enum proxy_entry_type direction,char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port);
void upstrem_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port);
void downstrem_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port);
void ofsw_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port);
void onu_flow_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port,int out_port);

void outport_controller_entry(char* buffer,int buf_len,int ofsw_sockfd,int in_port);
/*The 'Proxy_*' functions are used at create Flow_mod,Match,instruction,and action.
 *The 'Proxy_*' of functions are used to follow of rules:
 *						'Proxy_FlowMod'--> 'Proxy_Match' --> 'Proxy_instruction_actions'
 *						'Proxy_instruction_actions'-->Proxy_action_* -->'Proxy_instruction_goto_table'
 */
void Proxy_FlowMod(struct ofp_flow_mod **pflow_mod,uint8_t table_id,enum ofp_flow_mod_command cmd,uint16_t idle_timeout,uint16_t hard_timeout,uint32_t out_port,uint32_t out_group);
void Proxy_Match(struct ofp_flow_mod **pflow_mod,uint32_t oxm_type,int value);
void Proxy_instruction_actions(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,enum ofp_instruction_type type);
void Proxy_instruction_goto_table(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_goto_table  **instruction_goto_table,struct ofp_action_header **action,uint8_t table_id);
void Proxy_action_header(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,enum ofp_action_type type);
void Proxy_action_push(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,enum ofp_action_type type, uint16_t ethertype);
void Proxy_action_set_field(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,int value);
void Proxy_action_output(struct ofp_flow_mod **pflow_mod,struct ofp_instruction_actions  **instruction_actions,struct ofp_action_header **action,int value);

int flow_mod_offset(uint8_t**oxm_tlv,uint8_t*pOxm_tlv,int match_len);
void oxm_match_printf(uint8_t*pOxm_tlv);
uint8_t read_payload(uint8_t*pOxm_tlv,int payload_len);
uint8_t* least_oxm_address(uint8_t*pOxm_tlv,int length);

void modfiy_flow_entry(char* buffer,int buf_len,int ofsw_sockfd);
