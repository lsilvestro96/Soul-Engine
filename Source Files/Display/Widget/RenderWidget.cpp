#include "RenderWidget.h"
#include "GPGPU\GPUManager.h"
#include "CUDA\RenderWidget.cuh"
#include "Events\EventManager.h"
#include "Raster Engine/RasterBackend.h"
#include "Input/InputManager.h"
#include "GPGPU/GPUBuffer.h"

/*
 *    Constructor.
 *    @param [in,out]	cameraIn	If non-null, the camera in.
 */

RenderWidget::RenderWidget(Camera* cameraIn)
	: buffer(GPUManager::GetBestGPU()), accumulator(GPUManager::GetBestGPU()), extraData(GPUManager::GetBestGPU())
{
	camera = cameraIn;

	widgetJob = RasterBackend::CreateJob();

	samples = 1.0f;

	//attach shaders to render a quad and apply a texture
	widgetJob->AttachShaders({
		RasterBackend::CreateShader("Resources/Shaders/vertex-shader[Renderer].glsl",VERTEX_SHADER),
		RasterBackend::CreateShader("Resources/Shaders/fragment-shader[Renderer].glsl",FRAGMENT_SHADER)
	});


	//init all uniform data
	widgetJob->RegisterUniform(std::string("screen"));

	uint indices[6];
	float vertices[6 * 4];

	vertices[0] = -1.0f;
	vertices[1] = -1.0f;
	vertices[2] = 0.0f;
	vertices[3] = 1.0f;


	vertices[4] = 1.0f;
	vertices[5] = 1.0f;
	vertices[6] = 0.0f;
	vertices[7] = 1.0f;


	vertices[8] = -1.0f;
	vertices[9] = 1.0f;
	vertices[10] = 0.0f;
	vertices[11] = 1.0f;


	vertices[12] = 1.0f;
	vertices[13] = -1.0f;
	vertices[14] = 0.0f;
	vertices[15] = 1.0f;

	indices[0] = 0;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 0;
	indices[4] = 1;
	indices[5] = 3;

	widgetJob->UploadGeometry(vertices, sizeof(vertices), indices, sizeof(indices));

	iCounter = 1;
	integrate = false;
	currentSize = glm::uvec2(312, 720);

}

/* Destructor. */
RenderWidget::~RenderWidget()
{

}

/* Draws this object. */
void RenderWidget::Draw() {

	EventManager::Listen("Input", "SPACE", [&integrate = integrate, &time = time](keyState state) {
		double newTime = glfwGetTime();
		if (newTime - time > 0.3f) {
			integrate = !integrate;
			time = newTime;
		}
	});

	if (integrate) {
		Integrate(renderSize.x*renderSize.y, buffer, accumulator, extraData, iCounter);
		iCounter++;
	}
	else {
		iCounter = 1;
	}

	buffer.UnmapResources();
	buffer.BindData(0);
	widgetJob->Draw();

	//add the rayJob back in
	buffer.MapResources();

	//get job values
	widgetJob->SetUniform(std::string("screen"), renderSize);

	//TODO update id
	RayEngine::ModifyJob(rayJob, 0);
}

/* Recreate data. */
void RenderWidget::RecreateData() {

	//remove the rayJob if it exists
	RayEngine::RemoveJob(rayJob);

	uint jobsize = size.x*size.y;

	//create the new accumulation Buffer
	accumulator.resize(jobsize);

	buffer.resize(jobsize);

	extraData.resize(jobsize);


	if (currentSize != size) {
		currentSize = size;
		renderSize = size;
		widgetJob->SetUniform(std::string("screen"), renderSize);
	}

	//update the camera
	camera->aspectRatio = renderSize.x / (float)renderSize.y;
	camera->film.resolution = renderSize;

	//add the ray job with new sizes
	buffer.MapResources();

	//TODO update id
	rayJob = RayEngine::AddJob(RayCOLOUR, renderSize.x*renderSize.y, true, samples, 0, buffer, extraData);
}
