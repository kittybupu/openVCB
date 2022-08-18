
#include "openVCB.h"
#include <stdio.h>
#include <memory>
#include <chrono>
#include <vector>

int main() {
	using namespace std::chrono;

	auto proj = std::make_unique<openVCB::Project>();

	std::vector<std::pair<const char*, steady_clock::time_point>> times;
	times.reserve(100);

	// Read .vcb file
	times.push_back({ "File read", high_resolution_clock::now() });
	proj->readFromVCB("sampleProject.vcb");

	times.push_back({ "Project preprocess", high_resolution_clock::now() });
	const bool useGorder = false;
	proj->preprocess(useGorder);

	times.push_back({ "VMem assembly", high_resolution_clock::now() });
	proj->assembleVmem();

	printf("Loaded %d groups and %d connections.\n", proj->numGroups, proj->writeMap.nnz);
	printf("Simulating 1 million ticks into the future...\n");

	// Advance 1 million ticks
	times.push_back({ "Advance 1 million ticks", high_resolution_clock::now() });
	proj->tick(1000000);

	times.push_back({ "End", high_resolution_clock::now() });

	printf("\nDone.\n\n");

	for (size_t i = 0; i < times.size() - 1; i++) {
		auto start = times[i].second;
		auto end = times[i + 1].second;
		double elapsed = duration_cast<duration<double>>(end - start).count();
		printf("%s took %.3f seconds.\n", times[i].first, elapsed);
	}

	double simTime = duration_cast<duration<double>>
		(times[times.size() - 1].second - times[times.size() - 2].second).count();
	printf("Average TPS: %.1f\n\n", 1000000. / simTime);

	proj->dumpVMemToText("vmemDump.txt");
	printf("Final VMem contents dumped to vmemDump.txt!\n");
}