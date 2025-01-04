local leap = require 'leap'

local LLPipeline = {}

-- ***************************************************************************
--  Render types
-- ***************************************************************************
function LLPipeline.getRenderTypes()
    return leap.request('LLPipeline', {op = 'getRenderTypes'}).types
end

function LLPipeline.hasRenderType(type)
    return leap.request('LLPipeline', {op = 'hasRenderType', type = type}).value
end

function LLPipeline.toggleRenderTypes(types)
    local args= {op = 'toggleRenderTypes'}
    if type(types) ~= 'table' then
      args.types = {types}
    else
      args.types = types
    end
    return leap.request('LLPipeline', args)
end

function LLPipeline.disableAllRenderTypes()
    leap.send('LLPipeline', {op = 'disableAllRenderTypes'})
end

function LLPipeline.enableAllRenderTypes()
    leap.send('LLPipeline', {op = 'enableAllRenderTypes'})
end

-- ***************************************************************************
--  Render Info Displays
-- ***************************************************************************
function LLPipeline.getRenderInfoDisplays()
    return leap.request('LLPipeline', {op = 'getRenderInfoDisplays'}).displays
end

function LLPipeline.hasRenderInfoDisplay(display)
    return leap.request('LLPipeline', {op = 'hasRenderInfoDisplay', display = display}).value
end

function LLPipeline.toggleRenderInfoDisplays(displays)
    local args= {op = 'toggleRenderInfoDisplays'}
    if type(displays) ~= 'table' then
      args.displays = {displays}
    else
      args.displays = displays
    end
    return leap.request('LLPipeline', args)
end

function LLPipeline.disableAllRenderInfoDisplays()
    leap.send('LLPipeline', {op = 'disableAllRenderInfoDisplays'})
end

function LLPipeline.enableAllRenderInfoDisplays()
    leap.send('LLPipeline', {op = 'enableAllRenderInfoDisplays'})
end

-- ***************************************************************************
--  Render Features
-- ***************************************************************************
function LLPipeline.getRenderFeatures()
    return leap.request('LLPipeline', {op = 'getRenderFeatures'}).types
end

function LLPipeline.hasRenderFeature(feature)
    return leap.request('LLPipeline', {op = 'hasRenderFeature', feature = feature}).value
end

function LLPipeline.toggleRenderFeatures(features)
    local args= {op = 'toggleRenderFeatures'}
    if type(features) ~= 'table' then
      args.features = {features}
    else
      args.features = features
    end
    return leap.request('LLPipeline', args)
end

function LLPipeline.disableAllRenderFeatures()
    leap.send('LLPipeline', {op = 'disableAllRenderFeatures'})
end

function LLPipeline.enableAllRenderFeatures()
    leap.send('LLPipeline', {op = 'enableAllRenderFeatures'})
end

return LLPipeline
