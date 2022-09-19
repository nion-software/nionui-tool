#ifndef IMAGE_H
#define IMAGE_H

enum ImageFormat
{
    Format_ARGB32,
    Format_Indexed8,
    Format_ARGB32_Premultiplied,
};

class ImageInterface
{
public:
    virtual void create(unsigned int width, unsigned int height, ImageFormat image_type) = 0;
    virtual unsigned char *scanLine(unsigned int row) = 0;
    virtual const unsigned char *scanLine(unsigned int row) const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void setColorTable(const std::vector<unsigned int> &colorTable) = 0;
};

#endif
