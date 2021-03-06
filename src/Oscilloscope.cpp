/*
 *	Copyright (C) 2016  Hannes Haberl
 *
 *	This file is part of GLMViz.
 *
 *	GLMViz is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	GLMViz is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with GLMViz.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Oscilloscope.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>

Oscilloscope::Oscilloscope(const Module_Config::Oscilloscope& config, const unsigned o_id): size(0), id(o_id){
	init_crt();

	configure(config);
}

void Oscilloscope::draw(){
	sh_crt.use();
	v_crt.bind();

	glDrawArrays(GL_LINE_STRIP, 0, size);
}

void Oscilloscope::init_crt(){
	const char* vert_code =
	#include "shader/osc.vert"
	;

	GL::Shader vert(vert_code, GL_VERTEX_SHADER);

	const char* geom_code =
	#include "shader/osc.geom"
	;

	GL::Shader geom(geom_code, GL_GEOMETRY_SHADER);

	const char* frag_code =
	#include "shader/osc.frag"
	;

	GL::Shader frag(frag_code, GL_FRAGMENT_SHADER);

	try{
		sh_crt.link(vert, geom, frag);
	}
	catch(std::invalid_argument& e){
		std::cerr << "Can't link oscilloscope shader!" << std::endl << e.what() << std::endl;
	}

	v_crt.bind();

	b_crt_y.bind();
	GLint arg_y = sh_crt.get_attrib("y");
	glVertexAttribPointer(arg_y, 1, GL_SHORT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(arg_y);

	GL::VAO::unbind();
}

void Oscilloscope::configure(const Module_Config::Oscilloscope& ocfg){
	sh_crt();

	GLint i_scale = sh_crt.get_uniform("scale");
	glUniform1f(i_scale, ocfg.scale/32768.0);

	GLint i_color = sh_crt.get_uniform("line_color");
	glUniform4fv(i_color, 1, ocfg.color.rgba);

	GLint i_width = sh_crt.get_uniform("width");
	glUniform1f(i_width, ocfg.width);

	GLint i_sigma = sh_crt.get_uniform("sigma");
	glUniform1f(i_sigma, ocfg.sigma);

	GLint i_sigma_c = sh_crt.get_uniform("sigma_coeff");
	glUniform1f(i_sigma_c, ocfg.sigma_coeff);

	set_transformation(ocfg.pos);

	channel = ocfg.channel;
}

void Oscilloscope::resize_x_buffer(const size_t size){
	sh_crt();
	GLint i_length = sh_crt.get_uniform("length_1");
	glUniform1f(i_length, 1./size);
}

void Oscilloscope::set_transformation(const Module_Config::Transformation& t){
	glm::mat4 transformation = glm::ortho(t.Xmin, t.Xmax, t.Ymin, t.Ymax);

	sh_crt();

	GLint i_trans = sh_crt.get_uniform("trans");
	glUniformMatrix4fv(i_trans, 1, GL_FALSE, glm::value_ptr(transformation));
}

void Oscilloscope::update_buffer(Buffer<int16_t>& buffer){
	auto lock = buffer.lock();
	// resize x coordinate buffer if necessary
	if(size != buffer.size){
		size = buffer.size;
		resize_x_buffer(size);

		b_crt_y.bind();
		glBufferData(GL_ARRAY_BUFFER, size * sizeof(int16_t), &buffer.v_buffer[0], GL_DYNAMIC_DRAW);
	}else{
		b_crt_y.bind();
		glBufferSubData(GL_ARRAY_BUFFER, 0, size * sizeof(int16_t), &buffer.v_buffer[0]);
	}
}

void Oscilloscope::update_buffer(std::vector<Buffer<int16_t>>& buffers){
	if(buffers.size() > channel){
		update_buffer(buffers[channel]);
	}else{
		update_buffer(buffers[0]);
	}
}
