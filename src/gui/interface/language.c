#ifdef ENABLE_GUI
#include<stdlib.h>
#include"init_internal.h"
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"tools.h"
#include"language.h"
#define TAG "language"

static lv_obj_t*scr,*box,*sel,*btn_ok;

#ifndef ENABLE_UEFI
static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}
#endif

static void ok_action(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_ok||e!=LV_EVENT_CLICKED)return;
	uint16_t i=lv_dropdown_get_selected(sel);
	if(!languages[i].lang)return;
	const char*lang=lang_concat(&languages[i],true,true);
	tlog_debug("set language to %s",lang);
	lang_set(lang);
	#ifndef ENABLE_UEFI
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_LANGUAGE);
	strcpy(msg.data.data,lang);
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		int ex=(errno==0)?response.data.status.ret:errno;
		lv_create_ok_msgbox(scr,ok_msg_click,_("init control command failed: %s"),strerror(ex));
		lv_obj_del_async(box);
		return;
	}
	#endif
	guiact_do_back();
}

static void init_languages(){
	lv_dropdown_clear_options(sel);
	char*lang=lang_get_locale(NULL);
	uint16_t s=0;
	for(size_t i=0;languages[i].name;i++){
		struct language*l=&languages[i];
		lv_dropdown_add_option(sel,l->name,i);
		if(lang_compare(l,lang))s=i;
	}
	lv_dropdown_set_selected(sel,s);
}

static int language_get_focus(void*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,sel);
	lv_group_add_obj(gui_grp,btn_ok);
	return 0;
}

static int language_lost_focus(void*d __attribute__((unused))){
	lv_group_remove_obj(sel);
	lv_group_remove_obj(btn_ok);
	return 0;
}

void language_menu_draw(lv_obj_t*screen){
	scr=lv_create_opa_mask(screen);

	static lv_style_t bs;
	lv_style_init(&bs);
	lv_style_set_pad_all(&bs,LV_STATE_DEFAULT,gui_font_size);

	box=lv_page_create(scr,NULL);
	lv_obj_add_style(box,LV_PAGE_PART_BG,&bs);
	lv_obj_set_click(box,false);
	lv_obj_set_width(box,gui_sw/6*5);

	lv_obj_t*txt=lv_label_create(box,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
	lv_obj_set_width(txt,lv_page_get_scrl_width(box));
	lv_label_set_text(txt,_("Select language"));

	sel=lv_dropdown_create(box,NULL);
	lv_obj_set_width(sel,lv_page_get_scrl_width(box)-gui_dpi/10);
	lv_obj_align(sel,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	init_languages();

	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);

	btn_ok=lv_btn_create(box,NULL);
	lv_obj_add_state(btn_ok,LV_STATE_CHECKED);
	lv_obj_add_style(btn_ok,LV_STATE_DEFAULT,&btn_style);
	lv_obj_set_width(btn_ok,lv_page_get_scrl_width(box)-gui_dpi/10);
	lv_obj_align(btn_ok,sel,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	lv_obj_set_event_cb(btn_ok,ok_action);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	lv_obj_set_height(box,lv_obj_get_y(btn_ok)+lv_obj_get_height(btn_ok)+(gui_font_size*2)+gui_dpi/20);
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);

	guiact_register_activity(&(struct gui_activity){
		.name="language-menu",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.get_focus=language_get_focus,
		.lost_focus=language_lost_focus,
		.back=true,
		.page=scr
	});
}
#endif
