#ifndef GBUFFER_OUT_GLSL
#define GBUFFER_OUT_GLSL

#define GBUFFER_COLOUR   0
#define GBUFFER_AMBIENT  1
#define GBUFFER_POSITION 2
#define GBUFFER_NORMAL   3
#define GBUFFER_RMAO     4
#define GBUFFER_COUNT    5

out vec4 gbuffer_output[GBUFFER_COUNT];

void gbuffer_SetColour(vec4 color)
{
  gbuffer_output[GBUFFER_COLOUR] = color;
}

void gbuffer_SetAmbient(vec4 color)
{
  gbuffer_output[GBUFFER_AMBIENT] = color;
}

void gbuffer_SetPosition(vec3 position)
{
  gbuffer_output[GBUFFER_POSITION] = vec4(position, 1);
}

void gbuffer_SetNormal(vec3 nrm)
{
  gbuffer_output[GBUFFER_NORMAL] = vec4(nrm / 2 + vec3(0.5), 1);
}

void gbuffer_SetRMAO(vec4 rmao)
{
  gbuffer_output[GBUFFER_RMAO] = rmao;
}

#endif // GBUFFER_OUT_GLSL
