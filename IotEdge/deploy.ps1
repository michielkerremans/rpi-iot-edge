# Set variables
$tag = Get-Date -Format "yyyyMMddHHmmss"
$moduleJson = "workspace\iotedge-solution\modules\remotemodule\module.json"

# Read the module.json file
$json = Get-Content $moduleJson | Out-String | ConvertFrom-Json

# Update the version tag
$json.image.tag.version = $tag

# Write the updated JSON back to the file
$json | ConvertTo-Json -Depth 10 | Set-Content $moduleJson

Write-Host "Updated module.json with tag: $tag"

# Build, push, and deploy
docker-compose run --rm iotedge-dev iotedgedev solution build
docker-compose run --rm iotedge-dev iotedgedev solution push
docker-compose run --rm iotedge-dev iotedgedev iothub deploy -p 1 -n deployment_rpi_mk_$tag

Write-Host "Deployed with tag: $tag"