#pragma once

#define ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS \
"float                                                                              \n"\
"rounded_rect_coverage (vec2 p, vec4 bounds, float top_radius, float bottom_radius) \n"\
"{                                                                                  \n"\
"  if (p.x < bounds.x || p.x > bounds.z || p.y < bounds.y || p.y > bounds.w)        \n"\
"    return 0.0;                                                                    \n"\
"                                                                                   \n"\
"  float center_x;                                                                  \n"\
"  float center_y;                                                                  \n"\
"  float clip_radius;                                                               \n"\
"                                                                                   \n"\
"  if (p.y < bounds.y + top_radius) {                                               \n"\
"    clip_radius = top_radius;                                                      \n"\
"    center_y = bounds.y + top_radius;                                              \n"\
"    if (p.x < bounds.x + top_radius)                                               \n"\
"      center_x = bounds.x + top_radius;                                            \n"\
"    else if (p.x > bounds.z - top_radius)                                          \n"\
"      center_x = bounds.z - top_radius;                                            \n"\
"    else                                                                           \n"\
"      return 1.0;                                                                  \n"\
"  } else if (p.y > bounds.w - bottom_radius) {                                     \n"\
"    clip_radius = bottom_radius;                                                   \n"\
"    center_y = bounds.w - bottom_radius;                                           \n"\
"    if (p.x < bounds.x + bottom_radius)                                            \n"\
"      center_x = bounds.x + bottom_radius;                                         \n"\
"    else if (p.x > bounds.z - bottom_radius)                                       \n"\
"      center_x = bounds.z - bottom_radius;                                         \n"\
"    else                                                                           \n"\
"      return 1.0;                                                                  \n"\
"  } else {                                                                         \n"\
"    return 1.0;                                                                    \n"\
"  }                                                                                \n"\
"                                                                                   \n"\
"  vec2 delta = p - vec2(center_x, center_y);                                       \n"\
"  float dist_squared = dot(delta, delta);                                          \n"\
"                                                                                   \n"\
"  float outer_radius = clip_radius + 0.5;                                          \n"\
"  if (dist_squared >= (outer_radius * outer_radius))                               \n"\
"    return 0.0;                                                                    \n"\
"                                                                                   \n"\
"  float inner_radius = clip_radius - 0.5;                                          \n"\
"  if (dist_squared <= (inner_radius * inner_radius))                               \n"\
"    return 1.0;                                                                    \n"\
"                                                                                   \n"\
"  return outer_radius - sqrt(dist_squared);                                        \n"\
"}                                                                                  \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_VARS \
"uniform vec4 bounds; // x, y: top left; z, w: bottom right                         \n"\
"uniform float top_radius;                                                          \n"\
"uniform float bottom_radius;                                                       \n"\
"uniform vec2 pixel_step;                                                           \n"\
"uniform int skip;                                                                  \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS \
ROUNDED_CLIP_FRAGMENT_SHADER_VARS \
ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS

#define ROUNDED_CLIP_FRAGMENT_SHADER_CODE \
"if (skip == 0) {                                                                   \n"\
"  vec2 texture_coord = cogl_tex_coord0_in.xy / pixel_step;                         \n"\
"  float alpha =                                                                    \n"\
"    rounded_rect_coverage(texture_coord, bounds, top_radius, bottom_radius);       \n"\
"  cogl_color_out = cogl_color_out * alpha;                                         \n"\
"}                                                                                  \n"
