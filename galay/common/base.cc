#include "base.h"


const char* galay::html::Html404NotFound = "<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <title>404 Not Found</title>\
    <style>\
        body {\
            font-family: 'Arial', sans-serif;\
            background-color: #f7f9fc;\
            margin: 0;\
            padding: 0;\
            display: flex;\
            justify-content: center;\
            align-items: center;\
            height: 100vh;\
        }\
        .container {\
            text-align: center;\
            background-color: #ffffff;\
            padding: 40px;\
            border-radius: 8px;\
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);\
            max-width: 500px;\
            width: 100%;\
        }\
        .container h1 {\
            font-size: 100px;\
            margin: 0;\
            color: #ff6b6b;\
        }\
        .container h2 {\
            font-size: 24px;\
            margin: 10px 0;\
            color: #333;\
        }\
        .container p {\
            font-size: 16px;\
            color: #666;\
        }\
        .container a {\
            color: #007bff;\
            text-decoration: none;\
            font-weight: bold;\
        }\
        .container a:hover {\
            text-decoration: underline;\
        }\
        .image {\
            margin-top: 20px;\
        }\
        .image img {\
            width: 100px;\
            height: auto;\
        }\
    </style>\
</head>\
<body>\
    <div class=\"container\">\
        <h1>404</h1>\
        <h2>Oops! Page Not Found</h2>\
        <p>It looks like the page you are looking for does not exist or has been moved.</p>\
        <p><a href=\"/\">Return to Homepage</a></p>\
    </div>\
</body>\
</html>";

const char* galay::html::Html405MethodNotAllowed = "<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <title>405 Method Not Allowed</title>\
    <style>\
        body {\
            font-family: 'Arial', sans-serif;\
            background-color: #f7f9fc;\
            margin: 0;\
            padding: 0;\
            display: flex;\
            justify-content: center;\
            align-items: center;\
            height: 100vh;\
        }\
        .container {\
            text-align: center;\
            background-color: #ffffff;\
            padding: 40px;\
            border-radius: 8px;\
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);\
            max-width: 500px;\
            width: 100%;\
        }\
        .container h1 {\
            font-size: 80px;\
            margin: 0;\
            color: #ff6b6b;\
        }\
        .container h2 {\
            font-size: 24px;\
            margin: 10px 0;\
            color: #333;\
        }\
        .container p {\
            font-size: 16px;\
            color: #666;\
            line-height: 1.6;\
        }\
        .container a {\
            color: #007bff;\
            text-decoration: none;\
            font-weight: bold;\
        }\
        .container a:hover {\
            text-decoration: underline;\
        }\
        .image {\
            margin-top: 20px;\
        }\
        .image img {\
            width: 100px;\
            height: auto;\
        }\
    </style>\
</head>\
<body>\
    <div class=\"container\">\
        <h1>405</h1>\
        <h2>Oops! Method Not Allowed</h2>\
        <p>Sorry, the method you are trying to use is not allowed on this server.</p>\
        <p><a href=\"/\">Return to Homepage</a></p>\
    </div>\
</body>\
</html>";