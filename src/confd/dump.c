/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<string.h>
#include"str.h"
#include"list.h"
#include"logger.h"
#include"system.h"
#include"confd_internal.h"
#define TAG "confd"

static const char*conf_summary(struct conf*key){
	static char buf[256];
	memset(buf,0,256);
	snprintf(
		buf,255,
		"SAVE:%s,UID:%d,GID:%d,MODE:%04o",
		BOOL2STR(key->save),
		key->user,key->group,key->mode
	);
	return buf;
}

static int dump(struct conf*key,char*name){
	char path[PATH_MAX]={0};
	if(key->name[0]){
		if(!name[0])strcpy(path,key->name);
		else snprintf(path,PATH_MAX-1,"%s.%s",name,key->name);
	}
	if(key->type==TYPE_KEY){
		tlog_debug("  %s %s (key)\n",conf_summary(key),*path?path:"[ROOT]");
		list*p=list_first(key->keys);
		if(!p)return 0;
		do{dump(LIST_DATA(p,struct conf*),path);}while((p=p->next));
	}else switch(key->type){
		case TYPE_STRING:{
			int len=0;
			char x[256]={0},*p=VALUE_STRING(key);
			if(p){
				strncpy(x,p,252);
				len=strlen(p);
			}else strcpy(x,"(null)");
			tlog_debug("  %s %s = \"%s\"%s %d bytes (string)\n", conf_summary(key),path,x,len>252?"...":"",len);
		}break;
		case TYPE_INTEGER: tlog_debug("  %s %s = %lld (integer)\n",  conf_summary(key),path,(long long int)VALUE_INTEGER(key));break;
		case TYPE_BOOLEAN: tlog_debug("  %s %s = %s (boolean)\n",    conf_summary(key),path,BOOL2STR(VALUE_BOOLEAN(key)));break;
		default:           tlog_debug("  %s %s = (Unknown)\n",       conf_summary(key),path);break;
	}
	return 0;
}

int conf_dump_store(){
	tlog_debug("dump configuration store:");
	return dump(conf_get_store(),"");
}
