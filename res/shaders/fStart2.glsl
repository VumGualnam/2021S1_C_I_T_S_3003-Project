// This shader file contains changes made for 
// part of Part G, Part H, Part I and Part J
varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded

uniform sampler2D texture;

varying vec3 pos; // position from vertex shader
varying vec3 vertexN; // normal from vertex shader

// Transferred from vertex shader
uniform vec4 LightPosition1, LightPosition2, LightPosition3;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

// varying vec4 color1; // part of Part G, Changing 'varying' to 'uniform' as below

uniform vec3 color1, color2, color3; // part of Part G, Part I, Part J
uniform float light3pitch, light3yaw; // part of Part J
uniform float brightness1, brightness2, brightness3;

void main()
{
	// Transferred from vertex shader and change to vec4
	// globalAmbient is independent of distance from the light source
    vec4 globalAmbient = vec4(0.1, 0.1, 0.1, 1.0); 
	
	
	// vec3 N = vertexN; // 
	vec3 N = normalize(vertexN); //
	
	/////////////// start: Light1 ///////////////////
	// The vector to the light from the vertex    
    vec3 Lvec = LightPosition1.xyz - pos; 
	
	// Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector
	
	vec3 light1Source = color1 * brightness1;
	
	// Compute terms in the illumination equation
    vec4 ambient1 = vec4( light1Source * AmbientProduct, 1.0);
	
	float Kd = max( dot(L, N), 0.0 );
    vec4  diffuse1 = vec4( Kd*light1Source*DiffuseProduct, 1.0);
	
	float Ks = pow( max(dot(N, H), 0.0), Shininess );
	// vec3 specular1 = Ks * SpecularProduct;
	vec4 specular1 = vec4(Ks * SpecularProduct * brightness1, 1.0); // part of Part H, to increase the shininess towards white
	
	if (dot(L, N) < 0.0 ) {
	specular1 = vec4(0.0, 0.0, 0.0, 1.0);
    }
	
	vec4 color1 = vec4(color1, 1.0); // in order to avoid error when changing the values of color as a uniform variable color is read-only
	
	// start: Part G, Uncomment the following Part F code to see Part G
	// start: Part F
	// Position of first light
    vec3 lpos1 = LightPosition1.xyz;
	float light1Distance = sqrt(
		(lpos1[0]-pos[0])*(lpos1[0]-pos[0])+
		(lpos1[1]-pos[1])*(lpos1[1]-pos[1])+
		(lpos1[2]-pos[2])*(lpos1[2]-pos[2])
		);
	
	// Calculation for attenuation
    // float light1AttenFactor = 1.0/(1.0+0.1*light1Distance + 0.05*light1Distance*light1Distance);
	float light1AttenFactor = 1.0/(light1Distance*light1Distance);
	color1 = globalAmbient  + light1AttenFactor * (ambient1 + diffuse1 + specular1);
	// end: Part F
	// end: Part G, Uncomment the above Part F code to see Part G
	
	// start: part of Part H, excluding the specular component but add again to the gl_FragColor at the bottom to get a shininess that tends to white
	color1 = globalAmbient  + light1AttenFactor * (ambient1  + diffuse1);	
	// end: part of Part H
	/////////////// end: Light1 ///////////////////
	
	
	
	
	/////////////// start: Light2, part of Part I ///////////////////
	// Partly adapted from given code for light 1 
	// Position of second light
    vec3 lpos2 = LightPosition2.xyz;
	vec3 Lvec2 = lpos2 - pos; // light direction vector
	vec3 L2 = normalize(Lvec2); // normalize light direction vector
	vec3 H2 = normalize(L2 + E);
	
	vec3 light2Source = color2 * brightness2;
	
	// Compute terms in the illumination equation
    vec4 ambient2 = vec4(light2Source * AmbientProduct, 1.0);
	
	float Kd2 = max( dot(L2, N), 0.0);
	vec4  diffuse2 = vec4(Kd2 * light2Source * DiffuseProduct, 1.0);
	
	float Ks2 = pow( max(dot(N, H2), 0.0), Shininess);
	vec4  specular2 = vec4(Ks2 * SpecularProduct * brightness2, 1.0);
	
	if(dot(L2, N) < 0.0) {
    specular2 = vec4(0.0, 0.0, 0.0, 1.0);
    }
	
	vec4 color2 = vec4(color2, 1.0); // in order to avoid error when changing the values of color as a uniform variable color is read-only 
	color2 = ambient2  + diffuse2;  // directional that appears as sunlight when the y component is high
	/////////////// end: Light2, part of Part I ///////////////////
	
	
	/////////////// start: Light3, part of Part J ///////////////////
	// Partly adapted from given code for light 1 
	// start: part of Part J
	// Third light (splot light) 
	vec3 light3Dir;
    light3Dir.x = -cos(radians(light3yaw))*cos(radians(light3pitch));
    light3Dir.y = sin(radians(light3pitch));
    light3Dir.z = sin(radians(light3yaw))*cos(radians(light3pitch));
	
	vec3 lpos3 = LightPosition3.xyz;
	vec3 Lvec3 = lpos3 - pos;
	vec3 L3 = normalize( Lvec3 );
	
	// vec3 light3Source = color3 * brightness3;
	vec4 light3Source = vec4 (color3 * brightness3, 1.0);
	
	vec4 ambient3 = vec4(light3Source * vec4(AmbientProduct, 1.0));
	
	// Spotlight, Light3 angle of rotation
	float rotAngleL3 = dot(L3,normalize(light3Dir)); 
	float Kd3 = max(rotAngleL3, 0.0);
	vec4 diffuse3 = vec4(Kd3 * light3Source * vec4(DiffuseProduct, 1.0));
	
	float Ks3 = pow( max(rotAngleL3,0.0), Shininess);
	vec4 specular3 = vec4(Ks3 * SpecularProduct, 1.0);
	if(rotAngleL3 < 0.0){
    specular3 = vec4(0.0, 0.0, 0.0,1.0);
    }
	float light3Distance = sqrt(
		(lpos3[0]-pos[0])*(lpos3[0]-pos[0])+
		(lpos3[1]-pos[1])*(lpos3[1]-pos[1])+
		(lpos3[2]-pos[2])*(lpos3[2]-pos[2])
		);
	float light3AttenFactor = 1.0/(1.0+0.7*light3Distance + 1.8*light3Distance*light3Distance);
	
	vec4 color3 = vec4(color3, 1.0);
    if(rotAngleL3 > 0.9) {
       color3 = ambient3 + light3Distance*(diffuse3) * 0.80; //  vec4(1.0);
    }
	else {
		color3 = ambient3;
	}
	// end: part of Part J
	/////////////// end: Light3, part of Part J ///////////////////
	
	
    gl_FragColor = globalAmbient + 
		( color1 + color2 + color3 ) * texture2D( texture, texCoord * 2.0 ) + 
		specular1 * brightness1 + // part of Part H
		specular2 + 
		specular3 * light3AttenFactor;
}

