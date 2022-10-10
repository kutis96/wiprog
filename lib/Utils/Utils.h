#ifndef WIPROG_UTILS_H
#define WIPROG_UTILS_H

String toHumanReadableSize(uint32_t numberOfBytes)
{
    String fsize = "";
    if (numberOfBytes < 1024)
        fsize = String(numberOfBytes) + " B";
    else if (numberOfBytes < (1024 * 1024))
        fsize = String(numberOfBytes / 1024.0, 3) + " KiB";
    else if (numberOfBytes < (1024 * 1024 * 1024))
        fsize = String(numberOfBytes / 1024.0 / 1024.0, 3) + " MiB";
    else
        fsize = String(numberOfBytes / 1024.0 / 1024.0 / 1024.0, 3) + " GiB";
    return fsize;
}

String toHexString(uint64_t n, uint8_t bits)
{
    uint8_t nHexChars = (bits / 4) + ((bits % 4 > 0) ? 1 : 0);

    const char hexmap[] = "0123456789ABCDEF";
    String str = "";
    for (int i = nHexChars - 1; i >= 0; i -= 1)
    {
        str += hexmap[(n >> (4 * i)) & 0x0F];
    }
    return str;
}

std::map<String, String> mimetypes = {
    std::pair<String, String>(".html", "text/html"),
    std::pair<String, String>(".css", "text/css"),
    std::pair<String, String>(".png", "image/png"),
    std::pair<String, String>(".jpg", "image/jpeg"),
    std::pair<String, String>(".ico", "image/x-icon"),
    std::pair<String, String>(".gif", "image/gif"),
    std::pair<String, String>(".js", "application/javascript"),
    std::pair<String, String>(".bin", "application/octet-stream"),
};

String decodeMimeType(String path)
{
    String suffix = ".bin";
    if (path.lastIndexOf('.') != -1)
    {
        suffix = path.substring(path.lastIndexOf('.'));
    }

    std::map<String, String>::iterator it;
    it = mimetypes.find(suffix);
    if (it != mimetypes.end())
    {
        return it->second;
    }
    else
    {
        return "application/octet-stream";
    }
}

#endif