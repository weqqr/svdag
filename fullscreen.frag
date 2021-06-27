#version 450

in vec2 v_uv;

out vec4 color;

uniform sampler2D frame_texture;

void main()
{
    color = texture(frame_texture, v_uv);
}

