Get HSDK from https://web.archive.org/web/20060513190224/http://www.premisesystems.com/downloads/hsdkinstall.exe
Copy the files in the HSDK's include folder to the include folder here.
Copy the files in the HSDK's lib folder to the lib folder here.

Open driverutil.h and change line number 3531 from
		BYTE* pszLine;
to
		BYTE* pszLine = nullptr;
