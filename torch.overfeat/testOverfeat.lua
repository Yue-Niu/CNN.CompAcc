--
--  Copyright (c) 2014, Facebook, Inc.
--  All rights reserved.
--
--  This source code is licensed under the BSD-style license found in the
--  LICENSE file in the root directory of this source tree. An additional grant
--  of patent rights can be found in the PATENTS file in the same directory.
--
-- testLogger = optim.Logger(paths.concat(opt.save, 'test.log'))
testLogger = io.open(paths.concat(opt.save, 'test.log'), 'w+')
runLogger = io.open(paths.concat(opt.save, 'run.log'), 'w+')

local batchNumber
local top1_center, loss, top5_center
local timer = torch.Timer()

function test()
   print('==> doing epoch on validation data:')
   print("==> online epoch # " .. epoch)
   print('==> convolution kernel rank ' .. rank_)

   batchNumber = 0
   cutorch.synchronize()
   timer:reset()

   -- set the dropouts to evaluate mode
   model:evaluate()

   top1_center = 0
   top5_center = 0
   loss = 0
   for i = 1, math.ceil(nTest/opt.batchSize) do -- nTest is set in 1_data.lua
      local indexStart = (i-1) * opt.batchSize + 1
      local indexEnd = math.min(nTest, indexStart + opt.batchSize - 1)
      donkeys:addjob(
         -- work to be done by donkey thread
         function()
            local inputs, labels = testLoader:get(indexStart, indexEnd)
            return inputs, labels
         end,
         -- callback that is run in the main thread once the work is done
         testBatch
      )
   end

   donkeys:synchronize()
   cutorch.synchronize()

   top1_center = top1_center * 100 / nTest
   top5_center = top5_center * 100 / nTest
   loss = loss / nTest -- because loss is calculated per batch
   -- testLogger:add{
   --    ['% top1 accuracy (test set) (center crop)'] = top1_center,
   --    ['avg loss (test set)'] = loss
   -- }
   testLogger:write(string.format('ConvLayer: [%d], Epoch: [%d], Rank: [%d], Total Time(s): %.2f \t\t'
                          .. 'average loss: %.2f \t '
                          .. 'Top-1(%%):\t %.2f\t ',
                       layer_, epoch, rank_, timer:time().real, loss, top1_center))
   testLogger:write('\n')
   testLogger:write(string.format('ConvLayer: [%d], Epoch: [%d], Rank: [%d], Total Time(s): %.2f \t\t'
                          .. 'average loss: %.2f \t '
                          .. 'Top-5(%%):\t %.2f\t ',
                       layer_, epoch, rank_, timer:time().real, loss, top5_center))
   testLogger:write('\n')
   testLogger:flush()
   runLogger:write(string.format('ConvLayer: [%d], Epoch: [%d], Rank: [%d], Total Time(s): %.2f \t\t'
                          .. 'average loss: %.2f \t '
                          .. 'Top-1(%%):\t %.2f\t ',
                       layer_, epoch, rank_, timer:time().real, loss, top1_center))
   runLogger:write(string.format('ConvLayer: [%d], Epoch: [%d], Rank: [%d], Total Time(s): %.2f \t\t'
                          .. 'average loss: %.2f \t '
                          .. 'Top-5(%%):\t %.2f\t ',
                       layer_, epoch, rank_, timer:time().real, loss, top5_center))
   runLogger:write('\n\n')
   runLogger:flush()

   print(string.format('Epoch: [%d][TESTING SUMMARY] Total Time(s): %.2f \t'
                          .. 'average loss (per batch): %.2f \t '
                          .. 'accuracy [Center](%%):\t top-1 %.2f\t ',
                       epoch, timer:time().real, loss, top1_center))

   print('\n')
   print(string.format('Epoch: [%d][TESTING SUMMARY] Total Time(s): %.2f \t'
                          .. 'average loss (per batch): %.2f \t '
                          .. 'accuracy [Center](%%):\t top-5 %.2f\t ',
                       epoch, timer:time().real, loss, top5_center))
   print('\n')


end -- of test()
-----------------------------------------------------------------------------
local inputs = torch.CudaTensor()
local labels = torch.CudaTensor()

function testBatch(inputsCPU, labelsCPU)
   batchNumber = batchNumber + opt.batchSize

   inputs:resize(inputsCPU:size()):copy(inputsCPU)
   labels:resize(labelsCPU:size()):copy(labelsCPU)

   local outputs = model:forward(inputs)
   local err = criterion:forward(outputs, labels)
   cutorch.synchronize()
   local pred = outputs:float()

   loss = loss + err * outputs:size(1)

   local _, pred_sorted = pred:sort(2, true)
   for i=1,pred:size(1) do
      local g = labelsCPU[i]
      if pred_sorted[i][1] == g then top1_center = top1_center + 1 end
      if pred_sorted[i][1] == g or pred_sorted[i][2] == g or pred_sorted[i][3] == g or pred_sorted[i][4] == g or pred_sorted[i][5] == g then 
        top5_center = top5_center + 1 
      end 
   end
   if batchNumber % 1024 == 0 then
      print(('Epoch: Testing [%d], Loss: [%.2f], Accuracy: [%.2f](%%)----[%d/%d]'):format(epoch, loss/batchNumber, top1_center*100/batchNumber, batchNumber, nTest))
      runLogger:write(('Epoch: Testing [%d], Loss: [%.2f], Accuracy: [%.2f](%%)----[%d/%d]'):format(epoch, loss/batchNumber, top1_center*100/batchNumber, batchNumber, nTest))
      runLogger:write('\n')
      print(('Epoch: Testing [%d], Loss: [%.2f], Accuracy-5: [%.2f](%%)----[%d/%d]'):format(epoch, loss/batchNumber, top5_center*100/batchNumber, batchNumber, nTest))
      runLogger:write(('Epoch: Testing [%d], Loss: [%.2f], Accuracy-5: [%.2f](%%)----[%d/%d]'):format(epoch, loss/batchNumber, top5_center*100/batchNumber, batchNumber, nTest))
      runLogger:write('\n')
      runLogger:flush()
   end
end
