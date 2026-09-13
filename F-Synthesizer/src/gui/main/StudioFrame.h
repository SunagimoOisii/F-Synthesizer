#pragma once
#include <memory>
struct GLFWwindow;

namespace studio
{
// Owned by the GUI loop; native move/resize stays with Windows.
class WindowFrame
{
  public:
    static constexpr int minWidth = 1240, minHeight = 720;
    explicit WindowFrame(GLFWwindow* window);
    ~WindowFrame();
    void drawControls(float width);
    void resize(int width, int height);

  private:
    struct Native;
    std::unique_ptr<Native> native;
};
}
