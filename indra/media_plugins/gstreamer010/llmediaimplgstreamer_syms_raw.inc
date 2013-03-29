
// required symbols to grab
LL_GST_SYM(true, gst_pad_peer_accept_caps, gboolean, GstPad *pad, GstCaps *caps);
LL_GST_SYM(true, gst_buffer_new, GstBuffer*, void);
LL_GST_SYM(true, gst_buffer_set_caps, void, GstBuffer*, GstCaps *);
LL_GST_SYM(true, gst_structure_set_value, void, GstStructure *, const gchar *, const GValue*);
LL_GST_SYM(true, gst_init_check, gboolean, int *argc, char **argv[], GError ** err);
LL_GST_SYM(true, gst_message_get_type, GType, void);
LL_GST_SYM(true, gst_message_type_get_name, const gchar*, GstMessageType type);
LL_GST_SYM(true, gst_message_parse_error, void, GstMessage *message, GError **gerror, gchar **debug);
LL_GST_SYM(true, gst_message_parse_warning, void, GstMessage *message, GError **gerror, gchar **debug);
LL_GST_SYM(true, gst_message_parse_state_changed, void, GstMessage *message, GstState *oldstate, GstState *newstate, GstState *pending);
LL_GST_SYM(true, gst_element_set_state, GstStateChangeReturn, GstElement *element, GstState state);
LL_GST_SYM(true, gst_object_unref, void, gpointer object);
LL_GST_SYM(true, gst_object_get_type, GType, void);
LL_GST_SYM(true, gst_pipeline_get_type, GType, void);
LL_GST_SYM(true, gst_pipeline_get_bus, GstBus*, GstPipeline *pipeline);
LL_GST_SYM(true, gst_bus_add_watch, guint, GstBus * bus, GstBusFunc func, gpointer user_data);
LL_GST_SYM(true, gst_element_factory_make, GstElement*, const gchar *factoryname, const gchar *name);
LL_GST_SYM(true, gst_element_get_type, GType, void);
LL_GST_SYM(true, gst_static_pad_template_get, GstPadTemplate*, GstStaticPadTemplate *pad_template);
LL_GST_SYM(true, gst_element_class_add_pad_template, void, GstElementClass *klass, GstPadTemplate *temp);
LL_GST_SYM(true, gst_element_class_set_details, void, GstElementClass *klass, const GstElementDetails *details);
LL_GST_SYM(true, gst_caps_unref, void, GstCaps* caps);
LL_GST_SYM(true, gst_caps_ref, GstCaps *, GstCaps* caps);
//LL_GST_SYM(true, gst_caps_is_empty, gboolean, const GstCaps *caps);
LL_GST_SYM(true, gst_caps_from_string, GstCaps *, const gchar *string);
LL_GST_SYM(true, gst_caps_replace, void, GstCaps **caps, GstCaps *newcaps);
LL_GST_SYM(true, gst_caps_get_structure, GstStructure *, const GstCaps *caps, guint index);
LL_GST_SYM(true, gst_caps_copy, GstCaps *, const GstCaps * caps);
//LL_GST_SYM(true, gst_caps_intersect, GstCaps *, const GstCaps *caps1, const GstCaps *caps2);
LL_GST_SYM(true, gst_element_register, gboolean, GstPlugin *plugin, const gchar *name, guint rank, GType type);
LL_GST_SYM(true, _gst_plugin_register_static, void, GstPluginDesc *desc);
LL_GST_SYM(true, gst_structure_get_int, gboolean, const GstStructure *structure, const gchar *fieldname, gint *value);
LL_GST_SYM(true, gst_structure_get_value, G_CONST_RETURN GValue *, const GstStructure *structure, const gchar *fieldname);
LL_GST_SYM(true, gst_value_get_fraction_numerator, gint, const GValue *value);
LL_GST_SYM(true, gst_value_get_fraction_denominator, gint, const GValue *value);
LL_GST_SYM(true, gst_structure_get_name, G_CONST_RETURN gchar *, const GstStructure *structure);
LL_GST_SYM(true, gst_element_seek, bool, GstElement *, gdouble, GstFormat, GstSeekFlags, GstSeekType, gint64, GstSeekType, gint64);

// optional symbols to grab
LL_GST_SYM(false, gst_registry_fork_set_enabled, void, gboolean enabled);
LL_GST_SYM(false, gst_segtrap_set_enabled, void, gboolean enabled);
LL_GST_SYM(false, gst_message_parse_buffering, void, GstMessage *message, gint *percent);
LL_GST_SYM(false, gst_message_parse_info, void, GstMessage *message, GError **gerror, gchar **debug);
LL_GST_SYM(false, gst_element_query_position, gboolean, GstElement *element, GstFormat *format, gint64 *cur);
LL_GST_SYM(false, gst_version, void, guint *major, guint *minor, guint *micro, guint *nano);

// GStreamer 'internal' symbols which may not be visible in some runtimes but are still used in expanded GStreamer header macros - yuck!  We'll substitute our own stubs for these.
//LL_GST_SYM(true, _gst_debug_register_funcptr, void, GstDebugFuncPtr func, gchar* ptrname);
//LL_GST_SYM(true, _gst_debug_category_new, GstDebugCategory *, gchar *name, guint color, gchar *description);
