@set /p ID= Input the ID from the volumental URL: 
@set URL_LEFT=https://my.volumental.com/uploads/%ID%/left.bmf
@set URL_RIGHT=https://my.volumental.com/uploads/%ID%/right.bmf
wget %URL_LEFT% -P ./output/
wget %URL_RIGHT% -P ./output/
./bmf_to_obj.exe left.bmf left.obj
./bmf_to_obj.exe right.bmf right.obj
rmdir /S /Q "./output/"