import in3
import time

conf = in3.Config()
conf.chainId = str(in3.Chain.KOVAN)

eth = in3.Client(in3_config=conf).eth


filter = eth.new_block_filter()

count = 0
while(count<10):
    changes = eth.get_filter_changes(filter)
    time.sleep(10)
    print(changes)
    count+=1
    print(">>>")




