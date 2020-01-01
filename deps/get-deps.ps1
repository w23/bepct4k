$client = New-Object System.Net.WebClient
$script_dir = Split-Path $script:MyInvocation.MyCommand.Path
$script_dir += "\"

function download([string]$url, [string]$filename) {
    Write-Host Downloading $url to $filename
    $client.DownloadFile($url, $script_dir + $filename)
}

function downloadAndUnpack([string]$url, [string]$filename) {
    download $url $filename
    Write-Host Extracting $filename
    $filename = $script_dir + $filename
    Expand-Archive $filename -DestinationPath $script_dir -Force
}

download "https://github.com/laurentlb/Shader_Minifier/releases/download/1.1.6/shader_minifier.exe" "shader_minifier.exe"
downloadAndUnpack "http://crinkler.net/crinkler22.zip" "crinkler22.zip"
downloadANdUnpack "https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/win64/nasm-2.14.02-win64.zip" "nasm-2.14.02-win64.zip"