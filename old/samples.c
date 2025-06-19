/*
	This is a collection of old code samples that may still be of use to see
*/




//inital 2d render loop of screen cord triangles found initally in render.c in the draw function
//       for(int y_iter = start_y; y_iter <= end_y; y_iter++){
//           for(int x_iter = start_x; x_iter <= end_x; x_iter++){
//               // need to get baycords
//               vec2 v2;
//               vec2 point;
//               vec3 bayValues;
//               point[0] = (float)x_iter;
//               point[1] = (float)y_iter;
//               glm_vec2_sub(point,a_prime,v2);
//               calculateBayCords(v0, v1, v2, bayValues);
//               if(bayValues[0] < 0 || bayValues[1] < 0 || bayValues[2] < 0){
//                   continue; // not in triangle
//               }
//               float z_interp = bayValues[0] * a_z + bayValues[1] * b_z  + bayValues[2] * c_z;
			//float z_buffer_val = screen->z_buffer[y_iter * screen->width + x_iter];
//               // do z_buffering here, the z value here should be negative since from the camera perspective
//               // - z values are in front of it
//               if(z_interp > z_buffer_val){
//                   screen->z_buffer[y_iter * screen->width + x_iter] = z_interp;
//                   // update image buffer
//                   // todo
			//	screen->drawlingBuffer[y_iter * screen->width + x_iter] = (int)' ';
//               }
//           }
//       }