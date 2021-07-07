// "Transferred to fragment shader" code blocks are part of Part G
attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
// varying vec4 color1; // transferred to fragment shader

// uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct; // transferred to fragment shader
uniform mat4 ModelView;
uniform mat4 Projection;
// uniform vec4 LightPosition1; // transferred to fragment shader
// uniform vec4 LightPosition2; // transferred to fragment shader
// uniform float Shininess; // transferred to fragment shader 

// start: part of Part G
// declaring the variables outside the main function 
// so that they can be passed on to the fragment shader
varying vec3 pos;  
varying vec3 vertexN;
// end: part of Part G

void main()
{
    vec4 vpos = vec4(vPosition, 1.0);

    // Transform vertex position into eye coordinates
    pos = (ModelView * vpos).xyz;

	// Transferred to fragment shader
    // // The vector to the light from the vertex    
    // vec3 Lvec = LightPosition.xyz - pos;

	// Transferred to fragment shader
    // // Unit direction vectors for Blinn-Phong shading calculation
    // vec3 L = normalize( Lvec );   // Direction to the light // source
    // vec3 E = normalize( -pos );   // Direction to the eye/camera
    // vec3 H = normalize( L + E );  // Halfway vector

    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    // vec3 N = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );
	// Renaming before sending to fragment shader
	vertexN = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );

	// Transferred to fragment shader
    // // Compute terms in the illumination equation
    // vec3 ambient1 = AmbientProduct;
	
	// Transferred to fragment shader
    // float Kd = max( dot(L, N), 0.0 );
    // vec3  diffuse1 = Kd*DiffuseProduct;
	
	// Transferred to fragment shader
    // float Ks = pow( max(dot(N, H), 0.0), Shininess );
    // vec3  specular1 = Ks * SpecularProduct;
    
	// Transferred to fragment shader
    // if (dot(L, N) < 0.0 ) {
	// specular1 = vec3(0.0, 0.0, 0.0);
    // } 

	// Transferred to fragment shader
    // // globalAmbient is independent of distance from the light source
    // vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
	
	// Transferred to fragment shader
	// color.rgb = globalAmbient  + ambient1  + diffuse1 + specular1;
	
	// Transferred to fragment shader
	// // start: Part F
	// // Position of first light
    // vec3 lpos1 = LightPosition1.xyz;
	// float light1Distance = sqrt(
	//	(lpos1[0]-pos[0])*(lpos1[0]-pos[0])+
	//	(lpos1[1]-pos[1])*(lpos1[1]-pos[1])+
	//	(lpos1[2]-pos[2])*(lpos1[2]-pos[2])
	//	);
	
	// Transferred to fragment shader
	// // Calculation for attenuation
    // // float light1AttenFactor = 1.0/(1.0+0.1*light1Distance + 0.05*light1Distance*light1Distance);
	// float light1AttenFactor = 1.0/(light1Distance*light1Distance);
	// color1.rgb = globalAmbient  + light1AttenFactor * (ambient1 + diffuse1 + specular1);
	// color1.a = 1.0;
	// end: Part F
	
    gl_Position = Projection * ModelView * vpos;
    texCoord = vTexCoord;
}
