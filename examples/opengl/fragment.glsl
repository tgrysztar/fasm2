#version 330 core
in vec3 vCol;
out vec4 fragColor;
uniform float iTime;
uniform vec3 iResolution;
void mainImage(out vec4 o,vec2 i);
void main() { mainImage(fragColor,vCol.rb*iResolution.xy); }

// Heptamer by holtsetio
// https://www.shadertoy.com/view/3fXyRf
void mainImage( out vec4 O, vec2 I )
{
	O = iResolution.xyzz*.5;
	for ( I = (I - O.xy ) / O.y;
		O.w++ < 92.;
		I *= mat2(.62,.78,-.78,.62)
	) I.x += cos(9.*I.y+iTime) / 15.;
	O.xy = ++I / 2.;
}
