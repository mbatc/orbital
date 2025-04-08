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

void gbuffer_SetPosition(vec4 position)
{
  gbuffer_output[GBUFFER_POSITION] = position;
}

void gbuffer_SetNormal(vec4 nrm)
{
  gbuffer_output[GBUFFER_NORMAL] = nrm;
}

void gbuffer_SetRMAO(vec4 rmao)
{
  gbuffer_output[GBUFFER_RMAO] = rmao;
}

#endif // GBUFFER_OUT_GLSL
