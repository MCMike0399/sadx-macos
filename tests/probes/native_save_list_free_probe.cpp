#include "game/save_file_list.hpp"
#include <algorithm>
extern "C" void sadx_native_save_list_free_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+256,0);sadx::file_select::List head;
    for(unsigned n=0;n<i[0];++n){auto node=sadx::file_select::List(new sadx::file_select::ListNode{},[o,n](sadx::file_select::ListNode *p){o[2+o[1]++]=n;delete p;});if(n)node->file=sadx::file_select::FileInfo{"SonicDX01.snc",n};if(!head)head=node;else{auto tail=head;while(tail->next)tail=tail->next;tail->next=node;}}
    sadx::file_select::free_save_list(head);o[0]=head!=nullptr;
}
