#include "viewer.hpp"
#include <cstring>
#include <event/event.hpp>
#include <geometry/types.hpp>
#include <render/types.hpp>

namespace sword
{

namespace state
{

struct Vertex
{
    glm::vec3 pos;
};

static const std::array<Vertex, 3> vertices = 
{{
    {{-0.5, -0.5, 0}}, {{0.5, 0.5, 0}}, {{-0.5, 0.5, 0}}
}};

Viewer::Viewer(StateArgs sa, Callbacks cb) :
    BranchState{sa, cb, {
            {"load_model", opcast(Op::loadModel)},
            {"initialize", opcast(Op::initialize)},
            {"initialize2", opcast(Op::initialize2)}
            }}
{
    activate(opcast(Op::loadModel));
    activate(opcast(Op::initialize));
}

void Viewer::handleEvent(event::Event* event)
{
    if (event->getCategory() == event::Category::CommandLine)
    {
        auto option = extractCommand(event);
        if (!option) return;
        switch(opcast<Op>(*option))
        {
            case Op::loadModel: std::cout << "did it" << '\n'; break;
            case Op::initialize: initialize(); break;
            case Op::initialize2: initialize2(); break;
        }
    }
}

void Viewer::initialize()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
    bindings[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
    bindings[0].setBinding(0);
    bindings[0].setStageFlags(vk::ShaderStageFlagBits::eVertex);
    bindings[0].setDescriptorCount(1);

    geo::VertexInfo vertInfo{sizeof(Vertex), {offsetof(Vertex, pos)}};

    pushCmd(compileShader.request("vertex/basic.vert", "tri"));
    pushCmd(compileShader.request("fragment/blue.frag", "blue"));
    pushCmd(prepareRenderFrames.request());
    pushCmd(createDescriptorSetLayout.request("foo", bindings));
    pushCmd(createFrameDescriptorSets.request(std::vector<std::string>({"foo"})));
    pushCmd(createSwapchainRenderpass.request("swap"));
    pushCmd(createPipelineLayout.request("layout", std::vector<std::string> ({"foo"})));
    pushCmd(createGraphicsPipeline.request(
                "view", "layout", "tri", "blue", "swap", 
                vk::Rect2D({0, 0}, {800, 800}), vertInfo, vk::PolygonMode::eLine));
    pushCmd(addFrameUniformBuffer.request(sizeof(Xform), 0));
    pushCmd(bindUboData.request(&xform, sizeof(Xform), 0));
    pushCmd(requestBufferBlock.request(
                sizeof(vertices[0]) * vertices.size(), 
                command::RequestBufferBlock::Type::host, 
                &stagingVertBuffer));
//    pushCmd(requestBufferBlock.request(
//                sizeof(vertices), 
//                command::RequestBufferBlock::Type::device, 
//                deviceVertBuffer));
//    pushCmd(recordRenderCommand.request(0, std::vector<uint32_t>{0}));
//    pushCmd(openWindow.request());

    deactivate(opcast(Op::initialize));
    activate(opcast(Op::initialize2));

}

void Viewer::initialize2()
{
    memcpy(stagingVertBuffer->pHostMemory, vertices.data(), sizeof(vertices[0]) * vertices.size());

    render::DrawParms drawParms{stagingVertBuffer, vertices.size(), 0};

    pushCmd(createRenderLayer.request("swap", "swap", "view", drawParms));

    pushCmd(recordRenderCommand.request(0, std::vector<uint32_t>{0}));

    pushCmd(openWindow.request());

    deactivate(opcast(Op::initialize2));
}


} // namespace state

} // namespace sword