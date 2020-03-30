#include <render/shader.hpp>
#include <iostream>
#include <fstream>

namespace sword
{

namespace render
{

Shader::Shader(const vk::Device& device, std::string filepath) :
	device{device}
{
	loadFile(filepath);
	createModule();
	stageInfo.setPName("main");
	stageInfo.setModule(module);

	mapEntries.resize(4);
	mapEntries[0].setSize(sizeof(float));
	mapEntries[0].setOffset(0 * sizeof(float));
	mapEntries[0].setConstantID(0);

	mapEntries[1].setSize(sizeof(float));
	mapEntries[1].setOffset(1 * sizeof(float));
	mapEntries[1].setConstantID(1);

	mapEntries[2].setSize(sizeof(int));
	mapEntries[2].setOffset(2 * sizeof(int));
	mapEntries[2].setConstantID(2);

	mapEntries[3].setSize(sizeof(int));
	mapEntries[3].setOffset(3 * sizeof(int));
	mapEntries[3].setConstantID(3);

	specInfo.setPMapEntries(mapEntries.data());
	specInfo.setMapEntryCount(mapEntries.size());
	specInfo.setPData(&specData);
	specInfo.setDataSize(sizeof(SpecData));

	stageInfo.setPSpecializationInfo(&specInfo);

	std::cout << "Shader constructed" << std::endl;
}

Shader::~Shader()
{
	if (module)
		device.destroyShaderModule(module);
}

//movement is tricky. going to disallow it for now
//
//Shader::Shader(Shader&& other) :
//	device{other.device},
//	shaderCode{std::move(other.shaderCode)},
//	codeSize{other.codeSize},
//	module{std::move(other.module)},
//	stageInfo{std::move(other.stageInfo)},
//	mapEntries{std::move(other.mapEntries)},
//	specInfo{std::move(other.specInfo)},
//	specializationFloats{std::move(other.specializationFloats)}
//{
//	std::cout << "shader move ctor called" << std::endl;
//	other.module = nullptr;
//	specInfo.setPMapEntries(mapEntries.data());
//	specInfo.setPData(specializationFloats.data());
//}

const vk::PipelineShaderStageCreateInfo& Shader::getStageInfo() const
{
	return stageInfo;
}

void Shader::loadFile(std::string filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	//ate: start reading at end of file. allows us to get the size of the file
	//binary: read the file as binary file
	assert(file.is_open() && "Failed to load shader file");
	codeSize = (size_t) file.tellg();
	shaderCode.resize(codeSize);

	file.seekg(0);
	file.read(shaderCode.data(), codeSize);
	file.close();
}

void Shader::createModule()
{
	vk::ShaderModuleCreateInfo ci;
	ci.setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()));
	ci.setCodeSize(codeSize);
	module = device.createShaderModule(ci);
}

void Shader::setWindowResolution(const uint32_t w, const uint32_t h)
{
    specData.windowWidth = w;
    specData.windowHeight = h;
}

VertShader::VertShader(const vk::Device& device, std::string filepath) :
	Shader(device, filepath)
{
	stageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
}

FragShader::FragShader(const vk::Device& device, std::string filepath) :
	Shader(device, filepath)
{
	stageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
}

}; // namespace render

}; // namespace sword
