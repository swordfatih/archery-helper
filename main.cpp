////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <CNGui/CNGui.hpp>
#include <random>
#include <chrono>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <dwmapi.h>
#include <gdiplus.h>

/////////////////////////////////////////////////
/// \brief Encode to a format
///
/// \param format Format to encode in
/// \param pClsid Store format in
///
/////////////////////////////////////////////////
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;
	uint32_t  num = 0;
	uint32_t  size = 0;

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);

	if(size == 0)
    {
        return -1;
    }

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));

	if(pImageCodecInfo == NULL)
    {
        return -1;
    }

	GetImageEncoders(num, size, pImageCodecInfo);
	for(uint32_t j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}

	free(pImageCodecInfo);
	return 0;
}

/////////////////////////////////////////////////
/// \brief Screenshot using windows.h
///
/// \param rect_to_screen Rect to capture
///
/////////////////////////////////////////////////
void gdiscreen(sf::IntRect rect_to_screen)
{
	IStream* istream;

	CreateStreamOnHGlobal(NULL, true, &istream);

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	uint64_t gdiplusToken;

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	{
		HDC scrdc, memdc;
		HBITMAP membit;
		scrdc = GetDC(0);
		memdc = CreateCompatibleDC(scrdc);
		membit = CreateCompatibleBitmap(scrdc, rect_to_screen.width, rect_to_screen.height);

		SelectObject(memdc, membit);
		BitBlt(memdc, 0, 0, rect_to_screen.width, rect_to_screen.height, scrdc, rect_to_screen.left, rect_to_screen.top, SRCCOPY);

		Gdiplus::Bitmap bitmap(membit, NULL);

		CLSID clsid;
		GetEncoderClsid(L"image/png", &clsid);

		bitmap.Save(L"screen.png", &clsid, NULL); // To save the jpeg to a file
		bitmap.Save(istream, &clsid, NULL);

		DeleteObject(memdc);
		DeleteObject(membit);
		ReleaseDC(0,scrdc);
	}

	Gdiplus::GdiplusShutdown(gdiplusToken);

	istream->Release();
}

/////////////////////////////////////////////////
static std::mt19937 random_generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());

/////////////////////////////////////////////////
int main()
{
	sf::RenderWindow window(sf::VideoMode(200, 400), "");
    window.setFramerateLimit(60);
    window.setPosition({873, 240});

    CNGui::EventHandler handle_event;

    MARGINS margins;
	margins.cxLeftWidth = -1;

    SetWindowLong(window.getSystemHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
    DwmExtendFrameIntoClientArea(window.getSystemHandle(), &margins);

    CNGui::Box box("Fatih's Archery Helper", {}, sf::Vector2f(window.getSize()));
    box.getStyle().background_color.homogeneous({50, 50, 50});
    box.getStyle().main_color.homogeneous(sf::Color::Transparent);
    box.internal().setSize({0, 0});

    sf::RectangleShape line({6, 400});
    line.setFillColor({255, 0, 0, 100});

    /*sf::RectangleShape lazer({2, 400});
    lazer.setFillColor({0, 136, 220});*/

	bool grabbed_window = false;
    sf::Vector2i grabbed_window_offset;

    bool active = false;

	std::thread([&]()
    {
        while(window.isOpen())
        {
            gdiscreen(sf::IntRect(window.getPosition().x, window.getPosition().y, window.getSize().x, window.getSize().y));

            sf::Image image;
            image.loadFromFile("screen.png");

            for(uint16_t axis_x = 0; axis_x < image.getSize().x; ++axis_x)
            {
                for(uint16_t axis_y = 0; axis_y < image.getSize().y; ++axis_y)
                {
                    sf::Color pixel_color = image.getPixel(axis_x, axis_y);

                    if(pixel_color == sf::Color{180, 42, 31})
                    {
                        line.setPosition(axis_x - 2, 0);
                        line.setFillColor({255, 0, 0, 100});
                        active = true;
                    }

                    if(pixel_color == sf::Color{133, 31, 26} && active)
                    {
                        line.setFillColor({81, 255, 13, 100});
                    }
                }
            }
        }
    }).detach();

	while(window.isOpen())
	{
		handle_event.process(window);

        if(auto event = handle_event.get_if(sf::Event::KeyPressed); handle_event.get_if(sf::Event::Closed) || (event && event->key.code == sf::Keyboard::Escape))
        {
            window.close();
        }

        if(auto event = handle_event.get_if(sf::Event::MouseButtonPressed); event)
        {
            if(event->mouseButton.button == sf::Mouse::Left)
            {

                grabbed_window_offset = window.getPosition() - sf::Mouse::getPosition();
                grabbed_window = true;
            }
        }

        if(auto event = handle_event.get_if(sf::Event::MouseButtonReleased); event)
        {
            if(event->mouseButton.button == sf::Mouse::Left)
            {
                grabbed_window = false;
            }
        }

        if(handle_event.get_if(sf::Event::MouseMoved))
        {
            if(grabbed_window)
            {
                window.setPosition(sf::Mouse::getPosition() + grabbed_window_offset);
            }
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            line.setFillColor(sf::Color::Transparent);
            active = false;
        }

		box(window);

		SetWindowPos(window.getSystemHandle(), HWND_TOPMOST, window.getPosition().x, window.getPosition().y, window.getSize().x, window.getSize().y, SWP_SHOWWINDOW);

		window.clear(sf::Color::Transparent);
        window.draw(line);
        //window.draw(lazer);
		window.draw(box);
        window.display();
    }

    return 0;
}
