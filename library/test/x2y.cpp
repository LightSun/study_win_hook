#include <interception.h>
#include "utils.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

enum ScanCode
{
    SCANCODE_X   = 0x2D,
    SCANCODE_Y   = 0x15,
    SCANCODE_ESC = 0x01
};

static void test_mock_mouse();
static void test_keyboard();
extern int main_tets2();

int main()
{
    setbuf(stdout, NULL);
    return main_tets2();
    //test_keyboard();
    //return 0;
    InterceptionContext context;
    InterceptionDevice device;
    InterceptionKeyStroke stroke;

    raise_process_priority();

    context = interception_create_context();

    interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);

    while(interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0)
    {
        if(stroke.code == SCANCODE_X) {
            printf("SCANCODE_X");
            stroke.code = SCANCODE_Y;
        }

        interception_send(context, device, (InterceptionStroke *)&stroke, 1);

        if(stroke.code == SCANCODE_ESC){
            printf("SCANCODE_ESC");
            break;
        }
    }

    interception_destroy_context(context);

    return 0;
}
void test_mock_mouse(){
    InterceptionContext context = interception_create_context();


    InterceptionMouseStroke mouseStroke[3];

    memset(mouseStroke, 0, sizeof (InterceptionMouseStroke) * 3);

    // 鼠标移动到屏幕中间

    mouseStroke[0].flags = INTERCEPTION_MOUSE_MOVE_ABSOLUTE;

    mouseStroke[0].x = 65535 / 2; // 坐标取值范围是0-65535

    mouseStroke[0].y = 65535 / 2;

    // 点击鼠标右键

    mouseStroke[1].state = INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN;

    mouseStroke[2].state = INTERCEPTION_MOUSE_RIGHT_BUTTON_UP;

    interception_send(context, INTERCEPTION_MOUSE(0), (InterceptionStroke*)mouseStroke, _countof(mouseStroke));


    interception_destroy_context(context);
}
void test_keyboard(){
    InterceptionContext context = interception_create_context();


    InterceptionKeyStroke keyStroke[2];

    memset(keyStroke, 0, sizeof (keyStroke));

    keyStroke[0].code = MapVirtualKey('A', MAPVK_VK_TO_VSC);

    keyStroke[0].state = INTERCEPTION_KEY_DOWN;

    keyStroke[1].code = keyStroke[0].code;

    keyStroke[1].state = INTERCEPTION_KEY_UP;

    interception_send(context, INTERCEPTION_KEYBOARD(0), (InterceptionStroke*)keyStroke, _countof(keyStroke));


    interception_destroy_context(context);
}
