#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <sstream>
#include <time.h>
#include <vector>

#include <Lp3/Exception.h>
#include <Lp3/Engine/Gfx/BitmapReader.h>
#include <Lp3/Engine/Gfx/Image.h>
#include <Lp3/Engine/Gfx/Geometry.hpp>
#include <Lp3/Engine/Gfx/Pixel.h>
#include <Lp3/Engine/Gfx/PixelImage.h>
#include <Lp3/Engine/Resources/EnvVars.h>
#include <Lp3/Engine/Resources/InputFileStream.hpp>
#include <Lp3/Engine/Resources/ReadStreamPtr.h>
#include <Lp3/Engine/Gfx/Pixel.h>

#pragma comment (lib, "user32.lib")
#pragma comment (lib, "Gdi32.lib")

using namespace std;

using boost::optional;
using std::string;
using Lp3::Exception;
using Lp3::Engine::Gfx::BitmapReader;
using Lp3::Engine::Gfx::Image;
using Lp3::Engine::Gfx::Geometry::Coordinates2d;
using Lp3::Engine::Gfx::Pixel;
using Lp3::Engine::Gfx::PixelImage;
using Lp3::Engine::Resources::EnvVars;
using Lp3::Engine::Resources::InputFileStream;
using Lp3::Engine::Resources::ReadStreamPtr;
using Lp3::Engine::Gfx::Pixel;

class Console
{
private:
    HDC dc;

public:

    Console(HDC dc, unsigned int startX, unsigned int startY)
    :   dc(dc),
        startX(startX),
        startY(startY)
    {}

    unsigned int startX;
    unsigned int startY;

    void nPSet(const unsigned int x, const unsigned int y, const Pixel & rgb)
    {
        if (rgb.Alpha < 128) {
            return;
        }
        COLORREF COLOR= RGB(rgb.Red, rgb.Green, rgb.Blue);
        unsigned int _x = x * 2;
        unsigned int _y = y * 2;
        SetPixel(dc, _x + 25, _y + 40, COLOR);
        SetPixel(dc, _x + 25 + 1, _y + 40, COLOR);
        SetPixel(dc, _x + 25, _y + 40 + 1, COLOR);
        SetPixel(dc, _x + 25 + 1, _y + 40 + 1, COLOR);
    }

    virtual void PSet(const unsigned int x, const unsigned int y,
                      const Pixel & rgb)
    {
        if (rgb.Alpha < 128) {
            return;
        }
        COLORREF COLOR= RGB(rgb.Red, rgb.Green, rgb.Blue);
        SetPixel(dc, x + startX, y + startY, COLOR);
    }
};


struct ConsoleStuff
{

    HANDLE consoleHandle;
    CONSOLE_FONT_INFO font;
    COORD fontSize;

    ConsoleStuff() {
        consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (INVALID_HANDLE_VALUE == consoleHandle)
        {
            std::cerr << "Failure to get console handle.\n";
            throw Lp3::Exception("Coudn't get console handle.");
        }

        if (0 == GetCurrentConsoleFont(consoleHandle, TRUE, &font))
        {
            std::cerr << "Failure to get console font.\n";
            throw Lp3::Exception("Coudn't get console font.");
        }

        fontSize = GetConsoleFontSize(consoleHandle, font.nFont);
        if (fontSize.X == 0 || fontSize.Y == 0)
        {
            std::cerr << "Failure to get console font size.\n";
            throw Lp3::Exception("Coudn't get console font size.\n");
        }
    }

    std::pair<int, int> GetCoordinates(int LinesToSkip)
    {
        auto info = GetInfo();
        int X = info.dwCursorPosition.X * this->fontSize.X;
        // The stuff windows returns is relative to the entire console buffer.
        // To correct it, we take what appears to be the height of the visible
        // window and subtract it from the overall position, getting us where
        // we want to be.
        auto un = info.srWindow.Bottom - (info.srWindow.Bottom - info.srWindow.Top);
        int Y = ((info.dwCursorPosition.Y - LinesToSkip) - un);
        Y = Y * this->fontSize.Y;

        X += info.srWindow.Left;
        return std::make_pair(X, Y);
    }

    CONSOLE_SCREEN_BUFFER_INFO GetInfo()
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (0 == GetConsoleScreenBufferInfo(this->consoleHandle, &info))
        {
            std::cerr << "Failure to get buffer info.\n";
            throw Lp3::Exception("Coudn't get buffer info.");
        }
        return info;
    }

    Coordinates2d<size_t> GetFontSize()
    {
        return { size_t(fontSize.X), size_t(fontSize.Y) };
    }

    Coordinates2d<size_t> ToPixelCoordinates(Coordinates2d<size_t> textCoord)
    {
        auto info = GetInfo();
        int X = textCoord.X * this->fontSize.X;
        X += info.srWindow.Left;
        // The stuff windows returns is relative to the entire console buffer.
        // To correct it, we take what appears to be the height of the visible
        // window and subtract it from the overall position, getting us where
        // we want to be.
        auto adjust = info.srWindow.Bottom
                        - (info.srWindow.Bottom - info.srWindow.Top);
        int Y = textCoord.Y - adjust;
        Y = Y * this->fontSize.Y;

        return { size_t(X), size_t(Y) };
    }


    Coordinates2d<size_t> GetTextCoordinates()
    {
        auto info = GetInfo();
        return { size_t(info.dwCursorPosition.X),
                 size_t(info.dwCursorPosition.Y) };
    }

    size_t GetColumnWidth()
    {
        auto info = GetInfo();
        return info.srWindow.Right - info.srWindow.Left;
    }
};

PixelImage loadImage(const string & fileName)
{
    ReadStreamPtr input(new InputFileStream(fileName.c_str()));
    BitmapReader reader(input);
    PixelImage image(reader.Width(), reader.Height());
    Pixel colorKey(255, 0, 255, 0);
    reader.Read(image, colorKey);
    return image;
}

void drawCow(Console & console, const Coordinates2d<size_t> startCoordinates,
             const PixelImage & image)
{
    for (size_t y = 0; y < image.Height(); ++ y) {
        for (size_t x = 0; x < image.Width(); ++ x) {
            const auto pixel = image.GetPixel(x, y);
            //std::cout << "(" << (int) pixel.Red << ", "
            //    << (int) pixel.Green << ", " << (int) pixel.Blue << ")\n";
            console.PSet(startCoordinates.X + x, startCoordinates.Y + y, pixel);
        }
    }
}

void wordBubble(const string & text, const int desiredWidth)
{
    std::istringstream inputStream(text);
    std::vector<string> words{std::istream_iterator<string>(inputStream),
                              std::istream_iterator<string>()};

    int index = 0;
    const int width = desiredWidth - 10;

    auto startLine = [&]() {
        std::cout <<  "   | ";
        index = 0;
    };

    auto endLine = [&]() {
        auto size = width - index;
        if (size < 0)
        {
            size = 0;
        }
        std::cout << string(size, ' ');
        std::cout <<  " |\n";
        index = 0;
    };

    auto print = [&](string & w)
    {
        if (index > 0 && (index + w.length()) >= width)
        {
            if (w == " ")
            {
                return;  // Skip spaces.
            }
            endLine();
            startLine();
        }
        std::cout << w;
        index += w.size();
    };


    std::cout << "   .-" << string(width, '-') << "-.\n";
    startLine();
    for (int i = 0; i < words.size(); ++i)
    {
        if (i > 0) {
            print(string{" "});
        }
        print(words[i]);
    }
    endLine();
    const auto wl = (width / 2) - 2;
    std::cout << "   '-" << string(wl, '-')
              << ". ."
              << string(width - wl - 3, '-') << "-'\n";
    std::cout << "     " << string(wl, ' ');
    std::cout << "|/\n";

}

void cowsay(Console & drawConsole, const string & name,
            const optional<string> & text,
            const optional<int> & xOffset,
            bool noSkip)
{
    const auto image = loadImage(name + ".bmp");

    ConsoleStuff console;

    if (text)
    {
        auto wordWidth = image.Width() / console.GetFontSize().X;
        if (text.get().size() < wordWidth)
        {
            wordWidth = text.get().size();
        }
        wordBubble(text.get(), (image.Width() / console.GetFontSize().X));
    }

    const int linesToSkip = noSkip ? 0
                                   : image.Height() / console.GetFontSize().Y;

    for (size_t n = 0; n < linesToSkip; ++ n) {
        std::cout << "\n";
    }

    auto textCoordinates = console.GetTextCoordinates();
    auto c = console.ToPixelCoordinates(
        {textCoordinates.X + xOffset.get_value_or(0),
         textCoordinates.Y - linesToSkip});

    drawCow(drawConsole, c, image);
}


int main(int argc, const char * * argv)
{
    /*if (argc < 2)
    {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "cscp") << " text" ;
        return 1;
    }*/

    string image = "cow";
    bool expectImage = false;
    bool expectXOffSet = false;
    bool noSkip = false;
    bool seenImage = false;
    bool seenXOffset = false;
    string text;
    optional<int> xOffset;


    for(int i = 1; i < argc; ++ i)
    {
        if (expectImage)
        {
            image = argv[i];
            expectImage = false;
            seenImage = true;
        }
        else if (expectXOffSet)
        {
            xOffset = boost::lexical_cast<int>(argv[i]);
            expectXOffSet = false;
            seenXOffset = true;
        }
        else
        {
            if (i == 1 && strncmp("--help", argv[i], 6) == 0)
            {
                std::cout << "Usage: " << (argc > 0 ? argv[0] : "cscp")
                          << " [-c other-image-file ]"
                             " [--x-offset ##]"
                             " [--no-skip]"
                             " text" << std::endl
                          << "       Note: The environment variable "
                             "COWSAY_LOCATION specifies where images are "
                             "found.";
                return 1;
            }
            else if (!seenImage && strncmp("-c", argv[i], 2) == 0)
            {
                if (argc < (i + 2))
                {
                    std::cerr << "Error - no cow name found following -c.\n";
                    return 1;
                }
                expectImage = true;
            }
            else if (!noSkip && strncmp("--no-skip", argv[i], 9) == 0)
            {
                noSkip = true;
            }
            else if (!seenXOffset && strncmp("--x-offset", argv[i], 11) == 0)
            {
                if (argc < (i + 2))
                {
                    std::cerr << "Error - no number found after --x-offset.\n";
                    return 1;
                }
                expectXOffSet = true;
            }
            else
            {
                if (text.length() > 0)
                {
                    text += " ";
                }
                text += argv[i];
            }
        }
    }

    char * s = getenv("COWSAY_LOCATION");
    if (nullptr == s)
    {
        std::cerr << "COWSAY_LOCATION environment variable not set.\n";
        return 1;
    }
    EnvVars::Initialize(s);

    HWND myconsole = GetConsoleWindow();
    HDC mydc = GetDC(myconsole);

    Console console(mydc, 0, 0);
    int exitCode = 0;
    try
    {
        optional<string> oText;
        if (text.length() > 0)
        {
            oText = text;
        }
        cowsay(console, image, oText, xOffset, noSkip);
    } catch(...)
    {
        std::cerr << "Cow error...\n";
        exitCode = 1;
    }
    ReleaseDC(myconsole, mydc);

    return exitCode;
}
