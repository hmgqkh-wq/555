How to use this repo (quick):

1) Copy all files into your GitHub repo preserving paths:
   - manifest.json
   - CMakeLists.txt
   - src/wrapper.c
   - config/*.json
   - .github/workflows/build.yml

2) Push to the 'main' branch. GitHub Actions will run and produce artifact 'your_wrapper.zip'.

3) Download your_wrapper.zip from Actions -> Artifacts.

4) On your phone (Eden):
   - Extract the ZIP so the file layout *root* contains:
     manifest.json
     libvulkan.so
     config/  (folder)

   - Place the ZIP in Eden installer or use Eden UI "Install" to pick the ZIP.
   - Eden will install and enable the wrapper.

5) Logs will be written to:
   /storage/emulated/0/eden_wrapper_logs/vulkan_calls.txt

6) After running a game for several minutes, upload the logs to me.

Files to DELETE from repo after adding this set (recommended cleanup):
 - Any old wrapper build outputs in repo root like libvulkan.xeno_wrapper.so
 - Any duplicate manifests (keep only the single manifest.json above)
 - Remove old src/icd_alias.c or other interfering C sources if present (I replaced with a single small wrapper)

If anything fails during GitHub Actions, copy the workflow logs screenshot and send to me; I'll fix quickly.
