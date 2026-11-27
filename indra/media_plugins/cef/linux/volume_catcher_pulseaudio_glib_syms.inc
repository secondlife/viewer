#define G paSymbolGrabber

// required symbols to grab
LL_GRAB_SYM(G, true, pa_glib_mainloop_free, void, pa_glib_mainloop* g)
LL_GRAB_SYM(G, true, pa_glib_mainloop_get_api, pa_mainloop_api*, pa_glib_mainloop* g)
LL_GRAB_SYM(G, true, pa_glib_mainloop_new, pa_glib_mainloop *, GMainContext *c)

// optional symbols to grab

#undef G
