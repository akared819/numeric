set pagination off
run
if $_isvoid($_exitcode)
  frame 0
  info locals
  p precord->name
  p *ppn
  thread apply all bt full
end
