#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec3 aCol;
out vec3 vCol;
uniform float iTime;
void main(){
	float c=cos(iTime), s=sin(iTime);
	vec2 p = vec2(c*aPos.x - s*aPos.y, s*aPos.x + c*aPos.y);
	gl_Position = vec4(p,0.0,1.0);
	vCol = aCol;
}
