set outputPath="%~1"
set solutionDirPath="%~2"

REM copy assets
xcopy ..\Assets\ %outputPath%Assets /Y /I /E

REM copysdk
xcopy %solutionDirPath%Dependencies\PlayFabMultiplayerGDKPreview\bin\PlayFabMultiplayerGDK.dll %outputPath% /Y
xcopy "%GRDKLatest%ExtensionLibraries\PlayFab.Party.Cpp\Redist\CommonConfiguration\neutral\Party.dll" %outputPath% /Y
xcopy "%GRDKLatest%ExtensionLibraries\PlayFab.PartyXboxLive.Cpp\Redist\CommonConfiguration\neutral\PartyXboxLive.dll" %outputPath% /Y
xcopy "%GRDKLatest%ExtensionLibraries\Xbox.XCurl.API\Redist\CommonConfiguration\neutral\XCurl.dll" %outputPath% /Y