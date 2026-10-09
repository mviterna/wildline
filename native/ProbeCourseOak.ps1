param([int[]]$Yaw=@(180,270),[string]$Map='Neighborhood')
$ErrorActionPreference='Stop'
$taskJobs=@()
try{
    foreach($taskAngle in $Yaw){
        $taskName="oak-$Map-$taskAngle"
        $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/WorldUpgradeVerification/$taskName"
        $taskLog=Join-Path $PSScriptRoot "$taskName.log"
        $taskArgs='"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game /Game/Maps/'+$Map+" -WildlineHillSmoke -WildlineCourseLane=0 -WildlineCourseOakYaw=$taskAngle -NullRHI -NoSound -unattended -UseFixedTimeStep -FPS=60 -UserDir=`"$taskProfile`" -abslog=`"$taskLog`""
        $taskJobs+=@{Process=(Start-Process 'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $taskArgs -WindowStyle Hidden -PassThru);Log=$taskLog;Yaw=$taskAngle}
    }
    foreach($taskJob in $taskJobs){
        while(!$taskJob.Process.WaitForExit(30000)){if((Get-Date)-$taskJob.Process.StartTime -gt [TimeSpan]::FromMinutes(5)){throw 'Oak probe timed out'}}
        $taskText=Get-Content $taskJob.Log -Raw
        Write-Output "$Map yaw=$($taskJob.Yaw) PASS=$($taskText -match 'WILDLINE HILL SMOKE: PASS')"
        Write-Output (($taskText -split "`n" | Where-Object {$_ -match 'HILL BOT pos='}) -join "`n")
    }
}finally{foreach($taskJob in $taskJobs){if(!$taskJob.Process.HasExited){Stop-Process -Id $taskJob.Process.Id -ErrorAction SilentlyContinue}}}
