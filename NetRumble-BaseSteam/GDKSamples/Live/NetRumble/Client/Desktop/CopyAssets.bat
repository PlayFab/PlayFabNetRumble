set outputPath="%~1"
set solutionDirPath="%~2"

REM copy assets
xcopy ..\Assets\ %outputPath%Assets /Y /I /E

REM copy steam sdk
xcopy %solutionDirPath%Dependencies\SteamSDK\steam_appid.txt %outputPath% /Y
xcopy %solutionDirPath%Dependencies\SteamSDK\redistributable_bin\win64\steam_api64.dll %outputPath% /Y
xcopy %solutionDirPath%Dependencies\SteamSDK\public\steam\lib\win64\sdkencryptedappticket64.dll %outputPath% /Y