#include <fstream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define INFO_LOG_SIZE 2048 // 2 kb

static constexpr float PI = 3.1415926;
static constexpr float SENSITIVITY = 0.1;
static constexpr float MOVE_SPEED = 0.3;

void panic(const char* message)
{
    fprintf(stderr, "panic: %s\n", message);
    exit(1);
}

std::string read_text(const std::string& path)
{
    std::ifstream t(path);
    if (!t.good()) {
        panic("file not found");
    }
    std::string str((std::istreambuf_iterator<char>(t)), {});
    return str;
}

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;

    constexpr Vec3(float x, float y, float z)
        : x(x)
        , y(y)
        , z(z)
    {
    }

    static Vec3 from_euler_angles(float pitch, float yaw)
    {
        float x = cos(yaw) * cos(pitch);
        float y = sin(pitch);
        float z = sin(yaw) * cos(pitch);

        return Vec3(x, y, z);
    }

    Vec3& operator+=(Vec3 rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vec3& operator-=(Vec3 rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    Vec3& operator*(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    const float* ptr() const { return &x; }
};

static constexpr Vec3 UP = Vec3(0, 1, 0);

Vec3 cross(Vec3 lhs, Vec3 rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x};
}

float length(Vec3 lhs)
{
    return sqrt(lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z);
}

Vec3 normalize(Vec3 lhs)
{
    auto reciprocal = 1.0f / length(lhs);
    return {lhs.x * reciprocal, lhs.y * reciprocal, lhs.z * reciprocal};
}

std::ostream& operator<<(std::ostream& stream, const Vec3& vec)
{
    stream << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return stream;
}

struct GlobalState {
    Vec3 position;
    float pitch, yaw;
    double cursor_x, cursor_y;
} g_state;

GLFWwindow* create_window(int width, int height, const char* title)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        panic("failed to create window with OpenGL 4.5 context");
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        panic("failed to load OpenGL function pointers");
    }

    glEnable(GL_FRAMEBUFFER_SRGB);

    return window;
}

GLuint create_shader(const char* source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        int size = 0;
        char log[INFO_LOG_SIZE];
        glGetShaderInfoLog(shader, INFO_LOG_SIZE, &size, log);

        panic(log);
    }

    return shader;
}

GLuint create_program(const char* vertex_src, const char* fragment_src)
{
    GLuint vertex_shader = create_shader(vertex_src, GL_VERTEX_SHADER);
    GLuint fragment_shader = create_shader(fragment_src, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        int size = 0;
        char log[INFO_LOG_SIZE];
        glGetProgramInfoLog(program, INFO_LOG_SIZE, &size, log);

        panic(log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

GLuint create_compute_program(const char* compute_src)
{
    GLuint compute_shader = create_shader(compute_src, GL_COMPUTE_SHADER);

    GLuint program = glCreateProgram();

    glAttachShader(program, compute_shader);

    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        int size = 0;
        char log[INFO_LOG_SIZE];
        glGetProgramInfoLog(program, INFO_LOG_SIZE, &size, log);

        panic(log);
    }

    glDeleteShader(compute_shader);

    return program;
}

GLuint create_vertex_buffer_object()
{
    float data[] = {
        -1.0f, 3.0f, 0.0f, 2.0f, // Top left
        3.0, -1.0f, 2.0f, 0.0f, // Bottom right
        -1.0f, -1.0f, 0.0f, 0.0f // Bottom left
    };

    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(data), data, GL_DYNAMIC_STORAGE_BIT);

    return vbo;
}

GLuint create_vertex_array_object(GLuint vbo)
{
    GLuint vao;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);

    return vao;
}

GLuint create_texture(int width, int height)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    return texture;
}

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void render();
    GLFWwindow* get_window() const { return window; }

    void set_uniform(GLint location, Vec3 value);

private:
    GLFWwindow* window;
    GLuint fullscreen_program;
    GLuint raytrace_program;
    GLuint frame;
    GLuint vao;
    GLuint vbo;
};

Renderer::Renderer()
{
    auto frag_source = read_text("../fullscreen.frag");
    auto vert_source = read_text("../fullscreen.vert");
    auto comp_source = read_text("../raytrace-dag.comp");

    window = create_window(1280, 720, "view-dag");
    fullscreen_program = create_program(vert_source.c_str(), frag_source.c_str());
    raytrace_program = create_compute_program(comp_source.c_str());
    frame = create_texture(1280, 720);
    vbo = create_vertex_buffer_object();
    vao = create_vertex_array_object(vbo);
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &frame);
    glDeleteProgram(raytrace_program);
    glDeleteProgram(fullscreen_program);
    glfwDestroyWindow(window);
}

void Renderer::render()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glUseProgram(raytrace_program);
    glDispatchCompute(1280 / 16, 720 / 16, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glViewport(0, 0, width, height);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(fullscreen_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
}

void Renderer::set_uniform(GLint location, Vec3 value)
{
    glUseProgram(raytrace_program);
    glUniform3fv(location, 1, value.ptr());
}

float degrees_to_radians(float value)
{
    return value * PI / 180;
}

void on_cursor_move(GLFWwindow* window, double x, double y)
{
    auto dx = static_cast<float>(g_state.cursor_x - x);
    auto dy = static_cast<float>(g_state.cursor_y - y);

    g_state.cursor_x = x;
    g_state.cursor_y = y;

    g_state.pitch += dy * SENSITIVITY;
    g_state.yaw -= dx * SENSITIVITY;

    if (g_state.pitch > 85.5) {
        g_state.pitch = 85.5;
    } else if (g_state.pitch < -85.5) {
        g_state.pitch = -85.5;
    }
}

void init_state(Renderer& renderer)
{
    glfwSetInputMode(renderer.get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(renderer.get_window(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetCursorPosCallback(renderer.get_window(), on_cursor_move);
    g_state.position = {-5, -5, -5};
    g_state.pitch = 45;
    g_state.yaw = 45;

    glfwGetCursorPos(renderer.get_window(), &g_state.cursor_x, &g_state.cursor_y);
}

void main_loop(Renderer& renderer)
{
    while (!glfwWindowShouldClose(renderer.get_window()) && !glfwGetKey(renderer.get_window(), GLFW_KEY_ESCAPE)) {
        auto look_dir = normalize(Vec3::from_euler_angles(
            degrees_to_radians(g_state.pitch),
            degrees_to_radians(g_state.yaw)));

        auto right = normalize(cross(look_dir, UP));

        Vec3 movement{};

        if (glfwGetKey(renderer.get_window(), GLFW_KEY_W)) {
            movement += look_dir;
        }
        if (glfwGetKey(renderer.get_window(), GLFW_KEY_S)) {
            movement -= look_dir;
        }
        if (glfwGetKey(renderer.get_window(), GLFW_KEY_A)) {
            movement -= right;
        }
        if (glfwGetKey(renderer.get_window(), GLFW_KEY_D)) {
            movement += right;
        }
        if (glfwGetKey(renderer.get_window(), GLFW_KEY_SPACE)) {
            movement += UP;
        }
        if (glfwGetKey(renderer.get_window(), GLFW_KEY_LEFT_SHIFT)) {
            movement -= UP;
        }

        g_state.position += movement * MOVE_SPEED;

        renderer.set_uniform(1, g_state.position);
        renderer.set_uniform(2, look_dir);

        glfwPollEvents();
        renderer.render();
    }
}

int main()
{
    if (!glfwInit()) {
        panic("failed to initialize GLFW");
    }

    {
        Renderer renderer;
        init_state(renderer);
        main_loop(renderer);
    }

    glfwTerminate();
    return 0;
}
