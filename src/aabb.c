#include <aabb.h>

MMBB *MMBBListMakeRoom(MMBBList *list, size_t count){
	if (list->used+count > list->total){
		if (!list->total) list->total = 1;
		while (list->used+count > list->total) list->total *= 2;
		list->elements = realloc_or_die(list->elements,list->total*sizeof(*list->elements));
	}
	list->used += count;
	return list->elements+list->used-count;
}

void aabb_to_mmbb(AABB *a, MMBB *m){
	glm_vec3_sub(a->position,a->half_extents,m->min);
	glm_vec3_add(a->position,a->half_extents,m->max);
}

void expand_mmbb(MMBB *m, vec3 v){
	for (int i = 0; i < 3; i++){
		if (v[i] < 0) m->min[i] += v[i];
		else m->max[i] += v[i];
	}
}

void mmbb_get_center(MMBB *m, vec3 c){
	for (int i = 0; i < 3; i++){
		c[i] = m->min[i] + 0.5f*(m->max[i]-m->min[i]);
	}
}