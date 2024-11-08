#pragma once

#include <filesystem>

enum class ShadingModel {
    BLINN_OR_PHONG
};

namespace utils {
    namespace globals {
        namespace skybox_params {
            namespace fs = std::filesystem;
            const fs::path SKYBOX_RIGHT_IMG = "resources/skybox/right.jpg";
            const fs::path SKYBOX_LEFT_IMG = "resources/skybox/left.jpg";
            const fs::path SKYBOX_TOP_IMG = "resources/skybox/top.jpg";
            const fs::path SKYBOX_BOTTOM_IMG = "resources/skybox/bottom.jpg";
            const fs::path SKYBOX_FRONT_IMG = "resources/skybox/front.jpg";
            const fs::path SKYBOX_BACK_IMG = "resources/skybox/back.jpg";
            const auto SKYBOX_IMG_DATATYPE = GL_UNSIGNED_BYTE;
        }

        namespace shader_preprocessor_params {
            const int MAX_NUM_POINT_LIGHT = 4;
            const int MAX_NUM_DIR_LIGHT = 1;
            const int MAX_NUM_SPOT_LIGHT = 4;
        }

        const float lightPointSize = 15.0f;
        ShadingModel currentShadingModel = ShadingModel::BLINN_OR_PHONG;
        bool showLightsAsPoints = true;
        bool useBlinnCorrection = false;
        bool useDiffuseMap = true;
        bool useNormalMap = true;

        namespace bezier_path {
            const int frameCount = 360; // How many frames taken to do a full cubic bezier curve

            //const std::vector < glm::vec3 > control_points{
            //    { 3, 1, -1 }
            //    , { 2.5, 5,-1 }
            //    , { 2, 5, 1 }
            //    , { 1.5, 1, 1 }
            //    , { 1, -5, 1}
            //    , { 0.5, -5, -1}
            //    , { 0, 1, -1 }
            //};

            //const int curveCount = 2;

            //float timestep = 0;

            const glm::vec3 control_point0{ 3, 1, -2};
            const glm::vec3 control_point1{ 2.5, 5,-2 };
            const glm::vec3 control_point2{ 2, 5, 2 };
            const glm::vec3 control_point3{ 1.5, 1, 2 };
            const glm::vec3 control_point4{ 1, -5, 2 };
            const glm::vec3 control_point5{ 0.5, -5, -2 };
            const glm::vec3 control_point6{ 0, 1, -2 };
            const glm::vec3 control_point7{-0.5, 5, -2};
            const glm::vec3 control_point8{-1, 5, 2};
            const glm::vec3 control_point9{-1.5, 1, 2};

            const int curveCount = 3;

            float timestep = 0;
        }
    }

    namespace math {
        glm::vec2 getAttenuationCoefficient(float maxDistance) {
            if (maxDistance <= 7.0)
                return { 0.7, 1.8 };
            else if (maxDistance <= 13)
                return { 0.35, 0.44 };
            else if (maxDistance <= 20)
                return { 0.22, 0.2 };
            else if (maxDistance <= 32)
                return { 0.14, 0.07 };
            else if (maxDistance <= 50)
                return { 0.09, 0.032 };
            else if (maxDistance <= 65)
                return { 0.07, 0.017 };
            else if (maxDistance <= 100)
                return { 0.045, 0.0075 };
            else if (maxDistance <= 160)
                return { 0.027, 0.0028 };
            else if (maxDistance <= 200)
                return { 0.022, 0.0019 };
            else if (maxDistance <= 325)
                return { 0.014, 0.0007 };
            else if (maxDistance <= 600)
                return { 0.007, 0.0002 };
            else if (maxDistance <= 3250)
                return { 0.0014, 0.000007 };
            else
                return { 0.0001 , 0.0000001 };
        };

        glm::vec3 cubicBezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
            float it = 1.0f - t;
            return (it * it * it * p0) + (3 * t * it * it * p1) + (3 * t * t * it * p2) + (t * t * t * p3);
        }
    }

    static constexpr float skyboxVertices[] = {
        // Negative Z-axis: back
        -1.0f,  1.0f, -1.0f, // left-top-back
        -1.0f, -1.0f, -1.0f, // left-bottom-back
         1.0f, -1.0f, -1.0f, // right-bottom-back

         1.0f, -1.0f, -1.0f, // right-bottom-back
         1.0f,  1.0f, -1.0f, // right-top-back
        -1.0f,  1.0f, -1.0f, // left-top-back

        // Negative X-axis: left
        -1.0f, -1.0f,  1.0f, // left-bottom-front
        -1.0f, -1.0f, -1.0f, // left-bottom-back
        -1.0f,  1.0f, -1.0f, // left-top-back
                             
        -1.0f,  1.0f, -1.0f, // left-top-back
        -1.0f,  1.0f,  1.0f, // left-top-front
        -1.0f, -1.0f,  1.0f, // left-bottom-front

        // Positive X-axis: right
         1.0f, -1.0f, -1.0f, // right-bottom-back
         1.0f, -1.0f,  1.0f, // right-bottom-front
         1.0f,  1.0f,  1.0f, // right-top-front
                             
         1.0f,  1.0f,  1.0f, // right-top-front
         1.0f,  1.0f, -1.0f, // right-top-back
         1.0f, -1.0f, -1.0f, // right-bottom-back

         // Positive Z-axis: front
        -1.0f, -1.0f,  1.0f, // left-bottom-front
        -1.0f,  1.0f,  1.0f, // left-top-front
         1.0f,  1.0f,  1.0f, // right-top-front
                                
         1.0f,  1.0f,  1.0f, // right-top-front
         1.0f, -1.0f,  1.0f, // right-bottom-front
        -1.0f, -1.0f,  1.0f, // left-bottom-front

        // Positive Y-axis: top
        -1.0f,  1.0f, -1.0f, // left-top-back
         1.0f,  1.0f, -1.0f, // right-top-back
         1.0f,  1.0f,  1.0f, // right-top-front
                             
         1.0f,  1.0f,  1.0f, // right-top-front
        -1.0f,  1.0f,  1.0f, // left-top-front
        -1.0f,  1.0f, -1.0f, // left-top-back

        // Negative Y-axis: bottom
        -1.0f, -1.0f, -1.0f, // left-bottom-back
        -1.0f, -1.0f,  1.0f, // left-bottom-front
         1.0f, -1.0f, -1.0f, // right-bottom-back
                             
         1.0f, -1.0f, -1.0f, // right-bottom-back
        -1.0f, -1.0f,  1.0f, // left-bottom-front
         1.0f, -1.0f,  1.0f  // right-bottom-front
    };
}


