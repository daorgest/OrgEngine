#version 460

// Output color
layout(location = 0) out vec3 outColor;

void main()
{
    // Array of positions for the triangle vertices
    const vec3 vertexPositions[3] = vec3[3](
        vec3(1.0f, 1.0f, 0.0f),   // Top right vertex
        vec3(-1.0f, 1.0f, 0.0f),  // Top left vertex
        vec3(0.0f, -1.0f, 0.0f)   // Bottom vertex
    );

    // Array of colors for the triangle vertices
    const vec3 vertexColors[3] = vec3[3](
        vec3(1.0f, 0.0f, 0.0f),   // Red
        vec3(0.0f, 1.0f, 0.0f),   // Green
        vec3(0.0f, 0.0f, 1.0f)    // Blue
    );

    // Output the position of each vertex
    gl_Position = vec4(vertexPositions[gl_VertexIndex], 1.0f);

    // Output the color of each vertex
    outColor = vertexColors[gl_VertexIndex];
}