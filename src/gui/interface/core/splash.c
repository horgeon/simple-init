/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include<Protocol/BootLogo2.h>
#include<Library/UefiBootServicesTableLib.h>
#include"compatible.h"
#endif
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/splash.h"
#define TAG "splash"

static bool initialized=false,exited=true;
static lv_obj_t*img=NULL,*text=NULL,*scr=NULL;
static lv_color_t real_color;

#ifdef ENABLE_UEFI
static bool draw_edk2_logo(lv_obj_t*obj){
	size_t off;
	EFI_STATUS st;
	lv_align_t align;
	UINTN offx=0,offy=0;
	UINTN width=0,height=0;
	lv_color_t*buffer=NULL;
	lv_img_t*ext=(lv_img_t*)obj;
	EDKII_BOOT_LOGO2_PROTOCOL*bootlogo2=NULL;
	static lv_img_dsc_t dsc={.data=NULL,.data_size=0};
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*logoBitmap=NULL;
	if(dsc.data)free((void*)dsc.data);
	memset(&dsc,0,sizeof(dsc));
	st=gBS->LocateProtocol(&gEdkiiBootLogo2ProtocolGuid,NULL,(VOID**)&bootlogo2);
	if(EFI_ERROR(st)||!bootlogo2){
		tlog_debug(
			"get edk2 boot logo2 protocol failed: %s",
			efi_status_to_string(st)
		);
		return false;
	}
	st=bootlogo2->GetBootLogo(bootlogo2, &logoBitmap, &offx, &offy, &width, &height);
	if(EFI_ERROR(st)||!logoBitmap||width<=0||height<=0){
		tlog_warn(
			"cannot get logo image from edk2 protocol: %s",
			efi_status_to_string(st)
		);
		return false;
	}
	tlog_debug("logo image size %llux%llu",width,height);
	dsc.header.cf=LV_IMG_CF_TRUE_COLOR;
	dsc.header.w=width,dsc.header.h=height;
	dsc.data_size=LV_IMG_BUF_SIZE_TRUE_COLOR(dsc.header.w,dsc.header.h);
	if(!(buffer=malloc(dsc.data_size)))return false;
	dsc.data=(uint8_t*)buffer;
	memset(buffer,0,dsc.data_size);
	for(lv_coord_t y=0;y<dsc.header.h;y++){
		for(lv_coord_t x=0;x<dsc.header.w;x++){
			off=y*dsc.header.w+x;
			buffer[off].ch.red=logoBitmap[off].Red;
			buffer[off].ch.blue=logoBitmap[off].Blue;
			buffer[off].ch.green=logoBitmap[off].Green;
			buffer[off].ch.alpha=logoBitmap[off].Reserved;
		}
	}
	align=LV_ALIGN_CENTER;
	lv_img_set_src(obj,&dsc);
	lv_obj_align_to(obj,NULL,align,offx,offy);
	if(ext->w<=0||ext->h<=0){
		tlog_debug("draw logo failed");
		return false;
	}
	tlog_debug("logo draw successful");
	return true;
}
#endif

static void load_splash(lv_obj_t*obj){
	char*path;
	lv_img_t*ext=(lv_img_t*)obj;
	if((path=confd_get_string("gui.splash",NULL))){
		lv_img_set_src(obj,path);
		lv_obj_align_to(obj,NULL,LV_ALIGN_CENTER,0,0);
		free(path);
		if(ext->w>0&&ext->h>0)return;
	}
	#ifdef ENABLE_UEFI
	if(draw_edk2_logo(obj))return;
	#endif
}

int gui_splash_draw(){
	if(!lv_is_initialized())return -1;
	if(initialized||!exited)return 0;
	scr=lv_draw_wrapper(lv_scr_act(),NULL,NULL,gui_w,gui_h);
	real_color=lv_obj_get_style_bg_color(scr,0);
	lv_obj_set_style_bg_color(scr,lv_color_black(),0);
	lv_obj_set_style_bg_opa(scr,LV_OPA_COVER,0);

	img=lv_img_create(scr);
	load_splash(img);

	text=lv_label_create(scr);
	lv_obj_set_style_text_color(text,lv_color_white(),0);
	lv_obj_set_style_text_align(text,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(text,gui_w);
	lv_obj_set_pos(text,0,gui_h/5*4);
	lv_label_set_text(text,_("Initializing..."));

	initialized=true,exited=false;
	return 0;
}

void gui_splash_set_text(bool out,const char*fmt,...){
	char*str=NULL;
	if(!lv_is_initialized())return;
	if(!initialized||!text||!fmt)return;
	if(out)MUTEX_LOCK(gui_lock);
	va_list args;
	va_start(args,fmt);
	str=_lv_txt_set_text_vfmt(fmt,args);
	va_end(args);
	lv_label_set_text(text,str);
	lv_mem_free(str);
	if(out){
		lv_task_handler();
		lv_refr_now(NULL);
		MUTEX_UNLOCK(gui_lock);
	}
}

static void gui_splash_exit_done(void*d){
	lv_async_cb_t after_exit=d;
	lv_obj_del_async(scr);
	exited=true,img=NULL,text=NULL;
	if(after_exit)lv_async_call(after_exit,NULL);
}

static void anim_color_change(void*obj,int32_t val){
	lv_color_t cur;
	cur.ch.red=real_color.ch.red*val/255;
	cur.ch.green=real_color.ch.green*val/255;
	cur.ch.blue=real_color.ch.blue*val/255;
	lv_obj_set_style_bg_color(obj,cur,0);
}

static void anim_s2_ready_cb(lv_anim_t*anim){
	gui_splash_exit_done(lv_anim_get_user_data(anim));
}

static void anim_s1_ready_cb(lv_anim_t*anim){
	static lv_anim_t color_change;
	static bool done_img=false,done_text=false;
	if(anim->var==img)done_img=true;
	if(anim->var==text)done_text=true;
	if(!done_img||!done_text)return;
	done_img=false,done_text=false;
	lv_anim_init(&color_change);
	lv_anim_set_var(&color_change,lv_scr_act());
	lv_anim_set_time(&color_change,500);
	lv_anim_set_values(&color_change,0,255);
	lv_anim_set_exec_cb(&color_change,(lv_anim_exec_xcb_t)anim_color_change);
	lv_anim_set_ready_cb(&color_change,(lv_anim_ready_cb_t)anim_s2_ready_cb);
	lv_anim_set_user_data(&color_change,lv_anim_get_user_data(anim));
	lv_anim_start(&color_change);
}

static void on_gui_splash_exit(void*d){
	static lv_anim_t move_out_img;
	static lv_anim_t move_out_text;
	if(!initialized||!img||!text)return;
	initialized=false;
	if(!confd_get_boolean("gui.anim",true)){
		gui_splash_exit_done(d);
		return;
	}
	lv_obj_update_layout(img);
	lv_obj_update_layout(text);
	lv_anim_init(&move_out_img);
	lv_anim_init(&move_out_text);
	lv_anim_set_values(
		&move_out_img,
		lv_obj_get_y(img),
		-lv_obj_get_height(img)
	);
	lv_anim_set_values(
		&move_out_text,
		lv_obj_get_y(text),
		gui_h+lv_obj_get_height(text)
	);
	lv_anim_set_var(&move_out_img,img);
	lv_anim_set_var(&move_out_text,text);
	lv_anim_set_time(&move_out_img,500);
	lv_anim_set_time(&move_out_text,500);
	lv_anim_set_user_data(&move_out_img,d);
	lv_anim_set_user_data(&move_out_text,d);
	lv_anim_set_exec_cb(&move_out_img,(lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_exec_cb(&move_out_text,(lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_ready_cb(&move_out_img,(lv_anim_ready_cb_t)anim_s1_ready_cb);
	lv_anim_set_ready_cb(&move_out_text,(lv_anim_ready_cb_t)anim_s1_ready_cb);
	lv_anim_start(&move_out_img);
	lv_anim_start(&move_out_text);
}

void gui_splash_exit(bool out,lv_async_cb_t after_exit){
	if(!lv_is_initialized())return;
	if(!initialized)return;
	if(out)MUTEX_LOCK(gui_lock);
	lv_async_call(on_gui_splash_exit,after_exit);
	if(out){
		lv_task_handler();
		lv_refr_now(NULL);
		MUTEX_UNLOCK(gui_lock);
	}
}
