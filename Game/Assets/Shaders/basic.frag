#version 450

//in vec4 v_VertColor;

layout(location = 0) out vec4 f_color;

// TODO: add in a texture sampler

void main()
{
	// use the input vertex color
	f_color = vec4(1.0, 0.0, 0.0, 1.0);
}
