// This shader file contains changes made essentially for part of Part F, 
// no changes needed for Part A to Part E
attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec4 color1;

uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2; // part of Part F
uniform float Shininess;

void main()
{
    vec4 vpos = vec4(vPosition, 1.0);

    // Transform vertex position into eye coordinates
    vec3 pos = (ModelView * vpos).xyz;

    // The vector to the light from the vertex    
    vec3 Lvec = LightPosition1.xyz - pos;

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector

    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    vec3 N = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );

    // Compute terms in the illumination equation
    vec3 ambient1 = AmbientProduct;

    float Kd = max( dot(L, N), 0.0 );
    vec3  diffuse1 = Kd*DiffuseProduct;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    vec3  specular1 = Ks * SpecularProduct;
    
    if (dot(L, N) < 0.0 ) {
	specular1 = vec3(0.0, 0.0, 0.0);
    } 

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
	// color1.rgb = globalAmbient  + ambient1  + diffuse1 + specular1;
	
	// start: Part F
	// Position of first light
    vec3 lpos1 = LightPosition1.xyz;
	float light1Distance = sqrt(
		(lpos1[0]-pos[0])*(lpos1[0]-pos[0])+
		(lpos1[1]-pos[1])*(lpos1[1]-pos[1])+
		(lpos1[2]-pos[2])*(lpos1[2]-pos[2])
		);
	
	// Calculation for attenuation factor
    // float light1AttenFactor = 1.0/(1.0+0.1*light1Distance + 0.05*light1Distance*light1Distance);
	float light1AttenFactor = 1.0/(light1Distance*light1Distance);
	color1.rgb = globalAmbient  + light1AttenFactor * (ambient1 + diffuse1 + specular1);
	color1.a = 1.0;
	// end: Part F

    gl_Position = Projection * ModelView * vpos;
    texCoord = vTexCoord;
}
