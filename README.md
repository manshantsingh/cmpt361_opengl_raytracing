# Raytracing

This is class assignment to implement the phong illumination model. 
- We have plane with the texture of a chessboard.
- we have 3 ballon like spheres.
- The light equation includes shadows, reflections (including Diffuse reflections using stochastic ray generation) and refractions including Supersampling to reduce aliasing. These can be enabled based on the command line flags as follows:
	- +s: inclusion of shadows
	- +l: inclusion of reflection
	- +r: inclusion of refraction 
	- +c: inclusion of chess board pattern
	- +f: enabling diffuse rendering using stochastic ray generations 
	- +p: enabling super-sampling
- You also require either -u or -d flag to present user scene or default scene respectivily.
- step_max represents how many times the reflected and refracted rays are cast. 

Example command:
		./raycast –u  5 +s +l +r +f  +c +p
		This will enable all options and show the user scene with step_max = 5

For more infomation. See [Assignment instructions](http://www.cs.sfu.ca/~haoz/teaching/cmpt361/assign/a3/a3.pdf)
