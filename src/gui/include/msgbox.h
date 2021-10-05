#ifndef MSGBOX_H
#define MSGBOX_H
typedef bool(*msgbox_callback)(uint16_t id,const char*btn);
extern void msgbox_create_yesno(msgbox_callback callback,const char*content,...);
extern void msgbox_create_ok(msgbox_callback callback,const char*content,...);
extern void msgbox_create_custom(msgbox_callback callback,const char**btn,const char*content,...);
extern void msgbox_alert(const char*content,...);
#endif
