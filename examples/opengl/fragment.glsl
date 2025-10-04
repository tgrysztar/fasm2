#version 330 core
in vec3 vCol;
out vec4 FragColor;
void main(){ FragColor = vec4(vCol,1.0); }
