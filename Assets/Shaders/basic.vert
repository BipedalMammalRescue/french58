#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_Uv;
layout(location = 2) out vec3 v_ViewAngle;

layout(set = 1, binding = 0) uniform t_MVP 
{
	mat4 u_MVP;
};

layout(set = 1, binding = 1) uniform t_Camera
{
    vec3 u_CameraPos;
};

void main()
{
	gl_Position = u_MVP * vec4(position, 1.0);

	// forward the attributes into fragment shader 
	v_Normal = normal;
	v_Uv = uv;
    v_ViewAngle = normalize(u_CameraPos - position);
}
