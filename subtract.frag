uniform sampler2D backface;
uniform sampler2D frontface;
uniform sampler3D volume;

void main()
{
/* - Variables for my attempt, & box ---------------------------
   vec4 ray_entry, ray_exit;
   //vec4 direc;
   vec3 direc;
   
   ray_entry = texture2D (frontface, gl_TexCoord[0].st);
   ray_exit = texture2D (backface, gl_TexCoord[0].st); //stpq
*/   
	
/* - Proof that texture3D works (draws a slice) ----------------
   direc = vec4 (ray_entry.xyz - ray_exit.xyz, 1.0);
   gl_FragColor = direc;

   vec4 mri_slice;
   mri_slice = texture3D (volume, vec3(gl_TexCoord[0].st, 0.2));
   gl_FragColor = vec4 (mri_slice.rgb, 1.0);
*/

/* - My attempt at raycasting ----------------------------------
   vec3 origin, exit;
   
   origin = ray_entry.xyz;
   exit = ray_exit.xyz;
   //direc = normalize (exit - origin);
   direc = exit - origin;
   
   float t = 0.0, brightness = 0.0;
   float delta = 0.1;
   vec4 voxel, final_frag;
   
   //if (direc != vec3 (0.0, 0.0, 0.0))
   if (any (notEqual (direc, vec3 (0.0, 0.0, 0.0))))
   {
      //for (t = 0.0; (origin + direc * t) != exit; t += delta) //maintain compatibility w/ cards w/o shader model 3.0 support
      while (all (lessThanEqual (origin + direc * t, exit)))
      {
         voxel = texture3D (volume, origin + direc * t);
         brightness += 0.125 * voxel.r; //scale the voxel brightness by 1/8 to get deeper ray depth before termination
         if (brightness >= 1.0) 
         {
            if (brightness > 1.0) 
            {
               brightness = 1.0;
            }
            break;
         }
         t += delta;
      }
      final_frag.rgb = vec3 (brightness, brightness, brightness);
      
   } else {
      final_frag.rgb = vec3 (0.0, 0.0, 0.0);
   }
   final_frag.a = 1.0;
   
   gl_FragColor = final_frag;
   
   //P(t) = o + dir * t 
*/

/* - Box w/ brain textured sides -------------------------------
   vec3 origin = ray_entry.xyz;
   vec4 voxel;
   
   if (any (notEqual (origin, vec3 (0.0, 0.0, 0.0))))
   {
      voxel = texture3D (volume, origin);
      voxel.a = 1.0;
   } else {
      voxel = vec4 (0.0, 0.0, 0.0, 1.0);
   }
   gl_FragColor = voxel;
*/

/* - Direct adaptation of CG code from tutorial ----------------
   vec4 final_frag;
   vec2 texc = ((gl_FragCoord.xy / gl_FragCoord.w) + 1) / 2; //pixel coordinates
   vec4 start = gl_TexCoord[0].stpq;
   vec4 back_position = texture2D (backface, texc);
   vec3 dir = vec3(0.0, 0.0, 0.0);
   dir.x = back_position.x - start.x;
   dir.y = back_position.y - start.y;
   dir.z = back_position.z - start.z;
   float len = length(dir);
   vec3 norm_dir = normalize(dir);
   float delta = 0.1;
   vec3 delta_dir = norm_dir * delta;
   float delta_dir_len = length(delta_dir);
   vec3 vec = start.xyz;                       //differs
   vec4 col_acc = vec4(0.0, 0.0, 0.0, 0.0);
   float alpha_acc = 0.0;
   float length_acc = 0.0;
   vec4 color_sample;
   float alpha_sample;
   
   int i;
   for (i = 0; i < 450; i++)
   {
      color_sample = texture3D (volume, vec);
      alpha_sample = color_sample.a * 0.1;
      col_acc    += (1.0 - alpha_acc) * color_sample * alpha_sample * 3.0; //differs @ 3.0 instead of 3
      alpha_acc  += alpha_sample;
      vec        += delta_dir;
      length_acc += delta_dir_len;
      
      if (length_acc >= len || alpha_acc > 1.0)
      {
         break;
      }
   }
   final_frag = col_acc;
   gl_FragColor = final_frag;
*/

/* - Modified adaptation of CG code ---------------------------- */
   vec4 final_frag;

   vec4 start          = texture2D (frontface, gl_TexCoord[0].st);
   vec4 back_position  = texture2D (backface, gl_TexCoord[0].st); //stpq
   vec3 dir            = vec3(0.0, 0.0, 0.0);
   dir                 = back_position.xyz - start.xyz;
   if (all (equal (dir, vec3 (0.0, 0.0, 0.0))))
   {
      discard;
   }
   float len           = length(dir);
   vec3 norm_dir       = normalize(dir);
   float delta         = 0.025; //1/4 of 0.1
   vec3 delta_dir      = norm_dir * delta;
   float delta_dir_len = length(delta_dir);
   vec3 vec            = start.xyz;
   vec4 col_acc        = vec4(0.0, 0.0, 0.0, 0.0);
   float alpha_acc     = 0.0;
   float length_acc    = 0.0;
   vec4 color_sample;
   float alpha_sample;
   
   int i;
   for (i = 0; i < 450; i++)
   {
      color_sample = texture3D (volume, vec);
      alpha_sample = color_sample.a * delta;
      col_acc     += (1.0 - alpha_acc) * color_sample * alpha_sample * 3.0; //differs @ 3.0 instead of 3
      alpha_acc   += alpha_sample;
      vec         += delta_dir;
      length_acc  += delta_dir_len;
      
      if (length_acc >= len || alpha_acc > 1.0)
      {
         break;
      }
   }
   final_frag = col_acc;
   gl_FragColor = final_frag;
   
}
