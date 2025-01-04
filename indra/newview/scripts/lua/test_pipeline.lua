LLPipeline = require 'LLPipeline'
inspect = require 'inspect'
sleep = (require 'timers').sleep

print('List of available render types:\n' .. inspect(LLPipeline.getRenderTypes()))
LLPipeline.toggleRenderTypes('character')

print('List of available render features:\n' .. inspect(LLPipeline.getRenderFeatures()))
LLPipeline.toggleRenderFeatures('flexible')

print('List of available render info displays:\n' .. inspect(LLPipeline.getRenderInfoDisplays()))
LLPipeline.toggleRenderInfoDisplays('bboxes')

-- reset to default after 3 seconds
sleep(3)
LLPipeline.enableAllRenderTypes()
LLPipeline.enableAllRenderFeatures()
LLPipeline.disableAllRenderInfoDisplays()
