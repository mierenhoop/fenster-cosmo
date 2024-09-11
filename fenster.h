#ifndef FENSTER_H
#define FENSTER_H

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc-runtime.h>
//#elif defined(_WIN32)
//#include <windows.h>
#else
#define _DEFAULT_SOURCE 1
#define _COSMO_SOURCE
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#endif

#include <stdint.h>
#include <stdlib.h>

#include "libc/dce.h"
#include "libc/nexgen32e/nt2sysv.h"
#include "libc/nt/dll.h"
#include "libc/nt/enum/cw.h"
#include "libc/nt/enum/mb.h"
#include "libc/nt/enum/sw.h"
#include "libc/nt/enum/wm.h"
#include "libc/nt/enum/ws.h"
#include "libc/nt/enum/bi.h"
#include "libc/nt/enum/gwlp.h"
#include "libc/nt/enum/dib.h"
#include "libc/nt/enum/vk.h"
#include "libc/nt/enum/cs.h"
#include "libc/nt/enum/pm.h"
#include "libc/nt/enum/bitblt.h"
#include "libc/nt/events.h"
#include "libc/nt/messagebox.h"
#include "libc/nt/struct/msg.h"
#include "libc/nt/struct/wndclass.h"
#include "libc/nt/struct/bitmapinfoheader.h"
#include "libc/nt/struct/rgbquad.h"
#include "libc/nt/struct/paintstruct.h"
#include "libc/nt/struct/wndclassex.h"
#include "libc/nt/windows.h"
#include "libc/nt/paint.h"
#include "libc/nt/runtime.h"

#define HIWORD(l) ((uint16_t)((uint64_t)(l) >> 16))
#define LOWORD(l) ((uint16_t)((uint64_t)(l) & 0xffff))

struct fenster {
  const char *title;
  const int width;
  const int height;
  uint32_t *buf;
  int keys[256]; /* keys are mostly ASCII, but arrows are 17..20 */
  int mod;       /* mod is 4 bits mask, ctrl=1, shift=2, alt=4, meta=8 */
  int x;
  int y;
  int mouse;
#if defined(__APPLE__)
  id wnd;
#endif
//#elif defined(_WIN32)
  int64_t hwnd;
//#else
  xcb_connection_t *conn;
  uint32_t wid;
  uint32_t gc_;
  uint32_t pixmap;
//#endif
};

#ifndef FENSTER_API
#define FENSTER_API extern
#endif
FENSTER_API int fenster_open(struct fenster *f);
FENSTER_API int fenster_loop(struct fenster *f);
FENSTER_API void fenster_close(struct fenster *f);
FENSTER_API void fenster_sleep(int64_t ms);
FENSTER_API int64_t fenster_time(void);
#define fenster_pixel(f, x, y) ((f)->buf[((y) * (f)->width) + (x)])

#ifndef FENSTER_HEADER
#if defined(__APPLE__)
#define msg(r, o, s) ((r(*)(id, SEL))objc_msgSend)(o, sel_getUid(s))
#define msg1(r, o, s, A, a)                                                    \
  ((r(*)(id, SEL, A))objc_msgSend)(o, sel_getUid(s), a)
#define msg2(r, o, s, A, a, B, b)                                              \
  ((r(*)(id, SEL, A, B))objc_msgSend)(o, sel_getUid(s), a, b)
#define msg3(r, o, s, A, a, B, b, C, c)                                        \
  ((r(*)(id, SEL, A, B, C))objc_msgSend)(o, sel_getUid(s), a, b, c)
#define msg4(r, o, s, A, a, B, b, C, c, D, d)                                  \
  ((r(*)(id, SEL, A, B, C, D))objc_msgSend)(o, sel_getUid(s), a, b, c, d)

#define cls(x) ((id)objc_getClass(x))

extern id const NSDefaultRunLoopMode;
extern id const NSApp;

static void fenster_draw_rect(id v, SEL s, CGRect r) {
  (void)r, (void)s;
  struct fenster *f = (struct fenster *)objc_getAssociatedObject(v, "fenster");
  CGContextRef context =
      msg(CGContextRef, msg(id, cls("NSGraphicsContext"), "currentContext"),
          "graphicsPort");
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef provider = CGDataProviderCreateWithData(
      NULL, f->buf, f->width * f->height * 4, NULL);
  CGImageRef img =
      CGImageCreate(f->width, f->height, 8, 32, f->width * 4, space,
                    kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little,
                    provider, NULL, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(space);
  CGDataProviderRelease(provider);
  CGContextDrawImage(context, CGRectMake(0, 0, f->width, f->height), img);
  CGImageRelease(img);
}

static BOOL fenster_should_close(id v, SEL s, id w) {
  (void)v, (void)s, (void)w;
  msg1(void, NSApp, "terminate:", id, NSApp);
  return YES;
}

FENSTER_API int fenster_open(struct fenster *f) {
  msg(id, cls("NSApplication"), "sharedApplication");
  msg1(void, NSApp, "setActivationPolicy:", NSInteger, 0);
  f->wnd = msg4(id, msg(id, cls("NSWindow"), "alloc"),
                "initWithContentRect:styleMask:backing:defer:", CGRect,
                CGRectMake(0, 0, f->width, f->height), NSUInteger, 3,
                NSUInteger, 2, BOOL, NO);
  Class windelegate =
      objc_allocateClassPair((Class)cls("NSObject"), "FensterDelegate", 0);
  class_addMethod(windelegate, sel_getUid("windowShouldClose:"),
                  (IMP)fenster_should_close, "c@:@");
  objc_registerClassPair(windelegate);
  msg1(void, f->wnd, "setDelegate:", id,
       msg(id, msg(id, (id)windelegate, "alloc"), "init"));
  Class c = objc_allocateClassPair((Class)cls("NSView"), "FensterView", 0);
  class_addMethod(c, sel_getUid("drawRect:"), (IMP)fenster_draw_rect, "i@:@@");
  objc_registerClassPair(c);

  id v = msg(id, msg(id, (id)c, "alloc"), "init");
  msg1(void, f->wnd, "setContentView:", id, v);
  objc_setAssociatedObject(v, "fenster", (id)f, OBJC_ASSOCIATION_ASSIGN);

  id title = msg1(id, cls("NSString"), "stringWithUTF8String:", const char *,
                  f->title);
  msg1(void, f->wnd, "setTitle:", id, title);
  msg1(void, f->wnd, "makeKeyAndOrderFront:", id, nil);
  msg(void, f->wnd, "center");
  msg1(void, NSApp, "activateIgnoringOtherApps:", BOOL, YES);
  return 0;
}

FENSTER_API void fenster_close(struct fenster *f) {
  msg(void, f->wnd, "close");
}

// clang-format off
static const uint8_t FENSTER_KEYCODES[128] = {65,83,68,70,72,71,90,88,67,86,0,66,81,87,69,82,89,84,49,50,51,52,54,53,61,57,55,45,56,48,93,79,85,91,73,80,10,76,74,39,75,59,92,44,47,78,77,46,9,32,96,8,0,27,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,2,3,127,0,5,0,4,0,20,19,18,17,0};
// clang-format on
FENSTER_API int fenster_loop(struct fenster *f) {
  msg1(void, msg(id, f->wnd, "contentView"), "setNeedsDisplay:", BOOL, YES);
  id ev = msg4(id, NSApp,
               "nextEventMatchingMask:untilDate:inMode:dequeue:", NSUInteger,
               NSUIntegerMax, id, NULL, id, NSDefaultRunLoopMode, BOOL, YES);
  if (!ev)
    return 0;
  NSUInteger evtype = msg(NSUInteger, ev, "type");
  switch (evtype) {
  case 1: /* NSEventTypeMouseDown */
    f->mouse |= 1;
    break;
  case 2: /* NSEventTypeMouseUp*/
    f->mouse &= ~1;
    break;
  case 5:
  case 6: { /* NSEventTypeMouseMoved */
    CGPoint xy = msg(CGPoint, ev, "locationInWindow");
    f->x = (int)xy.x;
    f->y = (int)(f->height - xy.y);
    return 0;
  }
  case 10: /*NSEventTypeKeyDown*/
  case 11: /*NSEventTypeKeyUp:*/ {
    NSUInteger k = msg(NSUInteger, ev, "keyCode");
    f->keys[k < 127 ? FENSTER_KEYCODES[k] : 0] = evtype == 10;
    NSUInteger mod = msg(NSUInteger, ev, "modifierFlags") >> 17;
    f->mod = (mod & 0xc) | ((mod & 1) << 1) | ((mod >> 1) & 1);
    return 0;
  }
  }
  msg1(void, NSApp, "sendEvent:", id, ev);
  return 0;
}
#endif
//#elif defined(_WIN32)
// clang-format off
static const uint8_t FENSTER_KEYCODES[] = {0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,81,87,69,82,84,89,85,73,79,80,91,93,10,0,65,83,68,70,71,72,74,75,76,59,39,96,0,92,90,88,67,86,66,78,77,44,46,47,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,17,3,0,20,0,19,0,5,18,4,26,127};
// clang-format on
typedef struct BINFO{
    struct NtBitmapInfoHeader    bmiHeader;
    struct NtRgbQuad             bmiColors[3];
}BINFO;
static int64_t fenster_wndproc(int64_t hwnd, uint32_t msg, uint64_t wParam,
                                        int64_t lParam) {
  struct fenster *f = (struct fenster *)GetWindowLongPtr(hwnd, kNtGwlpUserdata);
  switch (msg) {
  case kNtWmPaint: {
    struct NtPaintStruct ps;
    int64_t hdc = BeginPaint(hwnd, &ps);
    int64_t memdc = CreateCompatibleDC(hdc);
    int64_t hbmp = CreateCompatibleBitmap(hdc, f->width, f->height);
    int64_t oldbmp = SelectObject(memdc, hbmp);
    BINFO bi = {{sizeof(bi), f->width, -f->height, 1, 32, kNtBiBitfields}};
    bi.bmiColors[0].rgbRed = 0xff;
    bi.bmiColors[1].rgbGreen = 0xff;
    bi.bmiColors[2].rgbBlue = 0xff;
    SetDIBitsToDevice(memdc, 0, 0, f->width, f->height, 0, 0, 0, f->height,
                      f->buf, (struct NtBitmapInfo *)&bi, kNtDibRgbColors);
    BitBlt(hdc, 0, 0, f->width, f->height, memdc, 0, 0, kNtSrccopy);
    SelectObject(memdc, oldbmp);
    DeleteObject(hbmp);
    DeleteDC(memdc);
    EndPaint(hwnd, &ps);
  } break;
  case kNtWmClose:
    DestroyWindow(hwnd);
    break;
  case kNtWmLbuttondown:
  case kNtWmLbuttonup:
    f->mouse = (msg == kNtWmLbuttondown);
    break;
  case kNtWmMousemove:
    f->y = HIWORD(lParam), f->x = LOWORD(lParam);
    break;
  case kNtWmKeydown:
  case kNtWmKeyup: {
    f->mod = ((GetKeyState(kNtVkControl) & 0x8000) >> 15) |
             ((GetKeyState(kNtVkShift) & 0x8000) >> 14) |
             ((GetKeyState(kNtVkMenu) & 0x8000) >> 13) |
             (((GetKeyState(kNtVkLwin) | GetKeyState(kNtVkRwin)) & 0x8000) >> 12);
    f->keys[FENSTER_KEYCODES[HIWORD(lParam) & 0x1ff]] = !((lParam >> 31) & 1);
  } break;
  case kNtWmDestroy:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}
#include <stdio.h>

static int fenster_open_win(struct fenster *f) {
  int64_t hInstance = GetModuleHandle(NULL);
  struct NtWndClassEx wc = {0};
  wc.cbSize = sizeof(struct NtWndClassEx);
  wc.style = kNtCsVredraw | kNtCsHredraw;
  wc.lpfnWndProc = NT2SYSV(fenster_wndproc);
  wc.hInstance = hInstance;
  // TODO: use f->title, but first convert it to char16_t
  static const char16_t title[] = u"hello";
  wc.lpszClassName = title;
  RegisterClassEx(&wc);
  f->hwnd = CreateWindowEx(kNtWsExClientedge, title, title,
                           kNtWsOverlappedwindow, kNtCwUsedefault, kNtCwUsedefault,
                           f->width, f->height, 0, 0, hInstance, 0);

  if (f->hwnd == 0)
    return -1;
  SetWindowLongPtr(f->hwnd, kNtGwlpUserdata, (int64_t)f);
  ShowWindow(f->hwnd, kNtSwNormal);
  UpdateWindow(f->hwnd);
  return 0;
}

static void fenster_close_win(struct fenster *f) { (void)f; }

static int fenster_loop_win(struct fenster *f) {
  struct NtMsg msg;
  while (PeekMessage(&msg, 0, 0, 0, kNtPmRemove)) {
    if (msg.dwMessage == kNtWmQuit)
      return -1;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  InvalidateRect(f->hwnd, NULL, 1);
  return 0;
}
//#else
// clang-format off
//static int FENSTER_KEYCODES[124] = {XK_BackSpace,8,XK_Delete,127,XK_Down,18,XK_End,5,XK_Escape,27,XK_Home,2,XK_Insert,26,XK_Left,20,XK_Page_Down,4,XK_Page_Up,3,XK_Return,10,XK_Right,19,XK_Tab,9,XK_Up,17,XK_apostrophe,39,XK_backslash,92,XK_bracketleft,91,XK_bracketright,93,XK_comma,44,XK_equal,61,XK_grave,96,XK_minus,45,XK_period,46,XK_semicolon,59,XK_slash,47,XK_space,32,XK_a,65,XK_b,66,XK_c,67,XK_d,68,XK_e,69,XK_f,70,XK_g,71,XK_h,72,XK_i,73,XK_j,74,XK_k,75,XK_l,76,XK_m,77,XK_n,78,XK_o,79,XK_p,80,XK_q,81,XK_r,82,XK_s,83,XK_t,84,XK_u,85,XK_v,86,XK_w,87,XK_x,88,XK_y,89,XK_z,90,XK_0,48,XK_1,49,XK_2,50,XK_3,51,XK_4,52,XK_5,53,XK_6,54,XK_7,55,XK_8,56,XK_9,57};
// clang-format on
static int fenster_open_xcb(struct fenster *f) {
  int defscr = 0;
  f->conn = xcb_connect(NULL, &defscr);
  assert(f->conn);
  xcb_screen_t *screen = NULL;
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(f->conn));
  for (; iter.rem; --defscr, xcb_screen_next(&iter)) {
    if (defscr == 0) {
      screen = iter.data;
      break;
    }
  }
  assert(screen);
  uint32_t mask = XCB_CW_EVENT_MASK;
  uint32_t values[1] = {
    XCB_EVENT_MASK_EXPOSURE
      | XCB_EVENT_MASK_KEY_PRESS
      | XCB_EVENT_MASK_KEY_RELEASE
      | XCB_EVENT_MASK_BUTTON_PRESS 
      | XCB_EVENT_MASK_BUTTON_RELEASE 
      | XCB_EVENT_MASK_POINTER_MOTION };
  f->wid = xcb_generate_id(f->conn);
  xcb_create_window(f->conn, XCB_COPY_FROM_PARENT, f->wid, screen->root, 0, 0, f->width, f->height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
  f->gc_ = xcb_generate_id(f->conn);
  xcb_create_gc(f->conn, f->gc_, f->wid, 0, NULL);
  xcb_change_property(f->conn, XCB_PROP_MODE_REPLACE, f->wid, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(f->title), f->title);
  xcb_map_window(f->conn, f->wid);
  xcb_flush(f->conn);
  return 0;
}
static void fenster_close_xcb(struct fenster *f) { xcb_disconnect(f->conn); }
static int fenster_loop_xcb(struct fenster *f) {
  xcb_generic_event_t *ev;
  // TODO: what is the scanline stuff (third zero)?
  xcb_put_image(f->conn, XCB_IMAGE_FORMAT_Z_PIXMAP, f->wid, f->gc_, f->width, f->height, 0, 0, 0, 24, 4*f->width*f->height, (uint8_t*)f->buf);
  xcb_flush(f->conn);
  while ((ev = xcb_poll_for_event(f->conn))) {
    switch (ev->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE:
      f->mouse = ((ev->response_type & ~0x80) == XCB_BUTTON_PRESS);
      break;
    case XCB_MOTION_NOTIFY:
      f->x = ((xcb_motion_notify_event_t *)ev)->event_x;
      f->y = ((xcb_motion_notify_event_t *)ev)->event_y;
      break;
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE: {
      // release is typedef'ed as press
      //int m = ((xcb_key_press_event_t *)ev)->state;
      // TODO: all following stuff
      //int k = XkbKeycodeToKeysym(f->dpy, ev.xkey.keycode, 0, 0);
      //for (unsigned int i = 0; i < 124; i += 2) {
      //  if (FENSTER_KEYCODES[i] == k) {
      //    f->keys[FENSTER_KEYCODES[i + 1]] = (ev.type == KeyPress);
      //    break;
      //  }
      //}
      //f->mod = (!!(m & ControlMask)) | (!!(m & ShiftMask) << 1) |
      //         (!!(m & Mod1Mask) << 2) | (!!(m & Mod4Mask) << 3);
    } break;
    }
    free(ev);
  }
  return 0;
}
//#endif

FENSTER_API int fenster_open(struct fenster *f) {
  if (IsWindows())
    return fenster_open_win(f);
  else
    return fenster_open_xcb(f);
}
FENSTER_API int fenster_loop(struct fenster *f) {
  if (IsWindows())
    return fenster_loop_win(f);
  else
    return fenster_loop_xcb(f);
}
FENSTER_API void fenster_close(struct fenster *f) {
  if (IsWindows())
    return fenster_close_win(f);
  else
    return fenster_close_xcb(f);
}

//#ifdef _WIN32
#if 0
FENSTER_API void fenster_sleep(int64_t ms) { Sleep(ms); }
FENSTER_API int64_t fenster_time() {
  LARGE_INTEGER freq, count;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&count);
  return (int64_t)(count.QuadPart * 1000.0 / freq.QuadPart);
}
#else
FENSTER_API void fenster_sleep(int64_t ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}
FENSTER_API int64_t fenster_time(void) {
  struct timespec time;
  clock_gettime(CLOCK_REALTIME, &time);
  return time.tv_sec * 1000 + (time.tv_nsec / 1000000);
}
#endif

#ifdef __cplusplus
class Fenster {
  struct fenster f;
  int64_t now;

public:
  Fenster(const int w, const int h, const char *title)
      : f{.title = title, .width = w, .height = h} {
    this->f.buf = new uint32_t[w * h];
    this->now = fenster_time();
    fenster_open(&this->f);
  }
  ~Fenster() {
    fenster_close(&this->f);
    delete[] this->f.buf;
  }
  bool loop(const int fps) {
    int64_t t = fenster_time();
    if (t - this->now < 1000 / fps) {
      fenster_sleep(t - now);
    }
    this->now = t;
    return fenster_loop(&this->f) == 0;
  }
  inline uint32_t &px(const int x, const int y) {
    return fenster_pixel(&this->f, x, y);
  }
  bool key(int c) { return c >= 0 && c < 128 ? this->f.keys[c] : false; }
  int x() { return this->f.x; }
  int y() { return this->f.y; }
  int mouse() { return this->f.mouse; }
  int mod() { return this->f.mod; }
};
#endif /* __cplusplus */

#endif /* !FENSTER_HEADER */
#endif /* FENSTER_H */
