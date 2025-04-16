#include "meta-clip-effect.h"
#include "meta/prefs.h"
#include "rounded-clip-shader.h"

typedef struct {
  CoglPipeline *pipeline;
  ClutterActor *actor;
  cairo_rectangle_int_t bounds;

  int bounds_uniform;
  int top_radius_uniform;
  int bottom_radius_uniform;
  int pixel_step_uniform;
  int skip_uniform;
} MetaClipEffectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(MetaClipEffect, meta_clip_effect, CLUTTER_TYPE_OFFSCREEN_EFFECT)

static CoglPipeline *
meta_clip_effect_class_create_pipeline(ClutterOffscreenEffect *effect,
                                       CoglTexture            *texture)
{
  MetaClipEffect *clip_effect = META_CLIP_EFFECT (effect);
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(clip_effect);
  cogl_pipeline_set_layer_texture (priv->pipeline, 0, texture);

  return cogl_object_ref (priv->pipeline);
}

static void
meta_clip_effect_set_actor(ClutterActorMeta *meta,
                           ClutterActor     *actor)
{
  ClutterActorMetaClass *meta_class 
    = CLUTTER_ACTOR_META_CLASS(meta_clip_effect_parent_class);
  MetaClipEffectPrivate *priv = 
    meta_clip_effect_get_instance_private(META_CLIP_EFFECT(meta));
  meta_class->set_actor(meta, actor);
  priv->actor = clutter_actor_meta_get_actor(meta);
}

static gboolean
meta_clip_effect_pre_paint (ClutterEffect *effect,
                            ClutterPaintContext *paint_context)
{
  gboolean res = 
    CLUTTER_EFFECT_CLASS (meta_clip_effect_parent_class)->pre_paint(effect, paint_context);
  MetaClipEffect *clip_effect = META_CLIP_EFFECT (effect);
  MetaClipEffectPrivate *priv = 
    meta_clip_effect_get_instance_private(META_CLIP_EFFECT(clip_effect));
  
  // seems CutterOffscreenEffect will set COGL_PIPELINE_FILTER_NEAREST
  // as layer filter, force set linear filter before paint now
  cogl_pipeline_set_layer_filters (priv->pipeline,
                                   0,
                                   COGL_PIPELINE_FILTER_LINEAR,
                                   COGL_PIPELINE_FILTER_LINEAR);
  return res;
}

static void
meta_clip_effect_dispose(GObject *gobject)
{
  MetaClipEffect *effect = META_CLIP_EFFECT(gobject);
  MetaClipEffectPrivate *priv = 
    meta_clip_effect_get_instance_private(META_CLIP_EFFECT(effect));

  if (priv->pipeline != NULL)
  {
    g_clear_pointer(&priv->pipeline, cogl_object_unref);
  }

  G_OBJECT_CLASS (meta_clip_effect_parent_class)->dispose (gobject);
}

static void
meta_clip_effect_class_init(MetaClipEffectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterEffectClass *effect_class = CLUTTER_EFFECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);
  ClutterActorMetaClass *meta_class = CLUTTER_ACTOR_META_CLASS(klass);

  meta_class->set_actor = meta_clip_effect_set_actor;
  effect_class->pre_paint = meta_clip_effect_pre_paint;
  offscreen_class->create_pipeline = meta_clip_effect_class_create_pipeline;
  gobject_class->dispose = meta_clip_effect_dispose;
}

static void
meta_clip_effect_init(MetaClipEffect *self)
{
  MetaClipEffectClass *klass = META_CLIP_EFFECT_GET_CLASS (self);
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(self);

  if (G_UNLIKELY (klass->base_pipeline == NULL))
    {
      CoglSnippet *snippet;
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());

      klass->base_pipeline = cogl_pipeline_new (ctx);

      snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT,
                                  ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS,
                                  ROUNDED_CLIP_FRAGMENT_SHADER_CODE);
      cogl_pipeline_add_snippet (klass->base_pipeline, snippet);
      cogl_object_unref (snippet);

      cogl_pipeline_set_layer_null_texture (klass->base_pipeline, 0);
    }

  priv->pipeline = cogl_pipeline_copy (klass->base_pipeline);
  priv->actor = NULL;

// get location of uniforms from shader
  priv->bounds_uniform = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "bounds");
  priv->top_radius_uniform =
    cogl_pipeline_get_uniform_location(priv->pipeline, "top_radius");
  priv->bottom_radius_uniform =
    cogl_pipeline_get_uniform_location(priv->pipeline, "bottom_radius");
  priv->pixel_step_uniform =
    cogl_pipeline_get_uniform_location(priv->pipeline, "pixel_step");
  priv->skip_uniform = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "skip");
}

MetaClipEffect *meta_clip_effect_new(void)
{
  return g_object_new(META_TYPE_CLIP_EFFECT, NULL);
}

void
meta_clip_effect_set_bounds(MetaClipEffect        *effect, 
                            cairo_rectangle_int_t *_bounds,
                            int                   padding[4])
{
  // padding: [left, right, top, bottom]

  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  g_return_if_fail(priv->pipeline && priv->actor);
  float top_radius = meta_prefs_get_top_corner_radius();
  float bottom_radius = meta_prefs_get_bottom_corner_radius();

  priv->bounds.x = _bounds->x + padding[0];
  priv->bounds.y = _bounds->y + padding[2];
  priv->bounds.width =  _bounds->width  - padding[1] - padding[0];
  priv->bounds.height = _bounds->height - padding[2] - padding[3];

  float x1 = priv->bounds.x;
  float y1 = priv->bounds.y;
  float x2 = priv->bounds.width + x1;
  float y2 = priv->bounds.height + y1;
  float w, h;

  clutter_actor_get_size(priv->actor, &w, &h);

  float bounds[] = { x1, y1, x2, y2 };
  float pixel_step[] = { 1. / w, 1. / h };

  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  priv->bounds_uniform,
                                  4, 1, bounds);
  cogl_pipeline_set_uniform_1f(priv->pipeline,
                               priv->top_radius_uniform,
                               top_radius);
  cogl_pipeline_set_uniform_1f(priv->pipeline,
                               priv->bottom_radius_uniform,
                               bottom_radius);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  priv->pixel_step_uniform,
                                  2, 1, pixel_step);
  cogl_pipeline_set_uniform_1i(priv->pipeline, priv->skip_uniform, 0);
}

void
meta_clip_effect_skip(MetaClipEffect *effect)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  g_return_if_fail(priv->pipeline && priv->actor);

  cogl_pipeline_set_uniform_1i(priv->pipeline, priv->skip_uniform, 1);
}

void
meta_clip_effect_get_bounds(MetaClipEffect        *effect,
                            cairo_rectangle_int_t *bounds)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);
  *bounds = priv->bounds;
}
