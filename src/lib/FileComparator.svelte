<script lang="ts">
  import { onMount } from 'svelte'; // 컴포넌트가 DOM에 마운트될 때 코드를 실행하기 위한 Svelte 함수
  import { diffLines } from 'diff'; // 두 텍스트의 차이점을 줄 단위로 비교하기 위한 라이브러리
  import { rooturl } from '../aqtstore'; // API 요청을 위한 기본 URL을 저장하는 Svelte 스토어

  // --- 상태 변수 선언 ---

  // SSH 연결 정보 (파일 내용 조회를 위해 사용)
  let host = 'localhost'; // 호스트 주소
  let port = 22; // 포트 번호
  let username = 'test'; // 사용자 이름
  let password = '1234'; // 비밀번호

  // Job 관련 상태
  let allJobIds = []; // 서버에서 가져온 모든 Job ID 목록
  let selectedJobId1 = ''; // 첫 번째 선택된 Job ID
  let selectedJobId2 = ''; // 두 번째 선택된 Job ID

  // 데이터 처리 및 UI 상태
  let comparisonResult = null; // 파일 비교 결과 객체
  let isLoading = false; // 비교 작업 진행 중 여부 (로딩 인디케이터 표시용)
  let errorMessage = ''; // 오류 발생 시 메시지
  let filterStatus = 'all'; // 파일 목록 필터 상태 ('all', 'added', 'modified', 'deleted')

  // 상세 정보 모달(팝업) 관련 상태
  let showModal = false; // 모달 표시 여부
  let modalContent = null; // 모달에 표시할 데이터 (파일 정보)
  let modalType = ''; // 모달 유형 ('added', 'deleted', 'modified')
  let oldFileContent = ''; // '삭제'된 파일의 내용
  let newFileContent = ''; // '신규' 파일의 내용
  let isContentLoading = false; // 파일 내용 로딩 중 여부
  let oldDiffHtml = ''; // '수정'된 파일의 이전 내용 (diff 하이라이팅된 HTML)
  let newDiffHtml = ''; // '수정'된 파일의 현재 내용 (diff 하이라이팅된 HTML)

  // 필터링된 파일 목록 (UI에 실제 표시될 목록)
  let filteredFiles1 = []; // 비교 대상 1의 필터링된 파일 목록
  let filteredFiles2 = []; // 비교 대상 2의 필터링된 파일 목록

  // --- 반응형 로직 ---

  // $: Svelte의 반응형 선언. comparisonResult나 filterStatus 값이 변경될 때마다 이 블록이 자동으로 실행됨.
  $: if (comparisonResult) {
    const modifiedOldPaths = new Set(comparisonResult.modified.map(item => transformPath(item.old.file_path)));
    const modifiedNewPaths = new Set(comparisonResult.modified.map(item => transformPath(item.new.file_path)));
    const deletedPaths = new Set(comparisonResult.deleted.map(item => transformPath(item.file_path)));
    const addedPaths = new Set(comparisonResult.added.map(item => transformPath(item.file_path)));

    // 필터 상태에 따라 보여줄 파일 목록을 업데이트
    switch (filterStatus) {
      case 'added':
        filteredFiles1 = [];
        filteredFiles2 = comparisonResult.added;
        break;
      case 'deleted':
        filteredFiles1 = comparisonResult.deleted;
        filteredFiles2 = [];
        break;
      case 'modified':
        filteredFiles1 = comparisonResult.modified.map(item => item.old);
        filteredFiles2 = comparisonResult.modified.map(item => item.new);
        break;
      case 'unchanged':
        filteredFiles1 = comparisonResult.files1.filter(f => !modifiedOldPaths.has(transformPath(f.file_path)) && !deletedPaths.has(transformPath(f.file_path)));
        filteredFiles2 = comparisonResult.files2.filter(f => !modifiedNewPaths.has(transformPath(f.file_path)) && !addedPaths.has(transformPath(f.file_path)));
        break;
      default: // 'all'
        filteredFiles1 = comparisonResult.files1;
        filteredFiles2 = comparisonResult.files2;
    }
  }

  // --- 생명주기 함수 ---

  // onMount: 컴포넌트가 처음 렌더링될 때 한 번 실행됨
  onMount(async () => {
    try {
      const response = await fetch(`${$rooturl}/execute-command/jobs/ids`);
      if (!response.ok) throw new Error('Job ID 목록을 불러오는데 실패했습니다.');
      allJobIds = await response.json();
    } catch (error) {
      errorMessage = error.message;
    }
  });

  // --- 함수 선언 ---

  // 파일 경로에서 특정 부분을 제거하여 비교를 위한 표준 경로로 변환하는 함수
  function transformPath(filePath) {
    if (!filePath) return '';
    const parts = filePath.split('/');
    // 경로가 /로 시작하고, 최소 3개의 디렉토리 구조를 가지는지 확인
    if (parts.length > 4 && parts[0] === '') { // e.g., ["", "home", "test", "tmp", "file.txt"]
      parts.splice(3, 1); // 3번째 디렉토리(index 3) 제거
    }
    return parts.join('/');
  }

  // '비교하기' 버튼 클릭 시 실행되는 메인 함수
  async function compareJobs() {
    if (!selectedJobId1 || !selectedJobId2) {
      errorMessage = '비교할 Job ID를 선택해주세요.';
      return;
    }
    if (selectedJobId1 === selectedJobId2) {
      errorMessage = '같은 JOB ID는  선택할수 없습니다.';
      return;
    }

    // 상태 초기화
    isLoading = true;
    errorMessage = '';
    comparisonResult = null;
    filterStatus = 'all'; // 비교 시 필터 초기화
    var jobid1 = selectedJobId1.split('|')[0];
    var jobid2 = selectedJobId2.split('|')[0];
    var tableSelect1 = selectedJobId1.split('|')[1];
    var tableSelect2 = selectedJobId2.split('|')[1];

    try {
      const response = await fetch(`${$rooturl}/execute-command/jobs/compare?jobId1=${jobid1}&jobId2=${jobid2}&tableSelect1=${tableSelect1}&tableSelect2=${tableSelect2}`);
      if (!response.ok) {
        const errData = await response.json();
        throw new Error(errData.message || '비교 데이터를 불러오는데 실패했습니다.');
      }
      const result = await response.json();

      // 두 파일 목록을 비교하여 '신규', '삭제', '수정' 상태를 계산
      const files1Map = new Map(result.files1.map(f => [transformPath(f.file_path), f]));
      const files2Map = new Map(result.files2.map(f => [transformPath(f.file_path), f]));

      const added = [];
      const deleted = [];
      const modified = [];

      // 비교 대상 2 (files2)를 기준으로 순회
      for (const [transformedPath, file2] of files2Map.entries()) {
        const file1 = files1Map.get(transformedPath);
        if (!file1) {
          added.push(file2); // file1에 없으면 '신규'
        } else {
          // 경로가 같지만 크기나 CRC 값이 다르면 '수정'
          if (file1.file_size !== file2.file_size || file1.crc_value !== file2.crc_value) {
            modified.push({ old: file1, new: file2 });
          }
          files1Map.delete(transformedPath); // 확인된 파일은 맵에서 제거
        }
      }
      // files1Map에 남아있는 파일들은 삭제된 파일들입니다.
      for (const file1 of files1Map.values()) {
        deleted.push(file1);
      }

      // 계산된 상태를 포함하여 최종 결과 저장
      comparisonResult = { ...result, added, deleted, modified };
    } catch (error) {
      errorMessage = error.message;
    } finally {
      isLoading = false;
    }
  }

  // 파일 목록에서 항목 클릭 시 상세 정보 모달을 여는 함수
  async function showDetails(item, type) {
    modalContent = item;
    modalType = type;
    showModal = true;
    oldFileContent = '';
    newFileContent = '';
    oldDiffHtml = '';
    newDiffHtml = '';

    // 파일 내용을 가져와야 하는 경우 (수정, 추가, 삭제)
    if (type === 'modified' || type === 'added' || type === 'deleted') {
      // Host와 Username이 입력되었는지 확인합니다.
      if (!host.trim() || !username.trim()) {
        alert('파일 내용을 보려면 상단의 Host와 Username 정보를 모두 입력해야 합니다.');
        closeModal(); // 팝업을 바로 닫아 입력을 유도합니다.
        return;
      }
      isContentLoading = true;
      try {
        if (type === 'modified') {
          // 이전 파일과 새 파일 내용 동시 요청
          const [oldRes, newRes] = await Promise.all([
            fetchFileContent(item.old.file_path),
            fetchFileContent(item.new.file_path)
          ]);
          const oldContent = oldRes.content || `(내용 없음) \n오류: ${oldRes.error || ''}`;
          const newContent = newRes.content || `(내용 없음) \n오류: ${newRes.error || ''}`;
          
          const diffs = diffLines(oldContent, newContent);
          
          let oldLineNum = 1;
          let newLineNum = 1;
          let oldHtml = '';
          let newHtml = '';

          diffs.forEach(part => {
            const lines = part.value.split('\n').slice(0, -1); // 마지막 빈 줄 제외
            if (part.added) {
              lines.forEach(line => {
                newHtml += `<div class="line-wrapper"><span class="line-num">${newLineNum++}</span><span class="line-content bg-green-200">${escapeHtml(line)}</span></div>`;
                oldHtml += `<div class="line-wrapper"><span class="line-num"></span><span class="line-content empty-line"></span></div>`;
              });
            } else if (part.removed) {
              lines.forEach(line => {
                oldHtml += `<div class="line-wrapper"><span class="line-num">${oldLineNum++}</span><span class="line-content bg-red-200">${escapeHtml(line)}</span></div>`;
                newHtml += `<div class="line-wrapper"><span class="line-num"></span><span class="line-content empty-line"></span></div>`;
              });
            } else {
              lines.forEach(line => {
                oldHtml += `<div class="line-wrapper"><span class="line-num">${oldLineNum++}</span><span class="line-content">${escapeHtml(line)}</span></div>`;
                newHtml += `<div class="line-wrapper"><span class="line-num">${newLineNum++}</span><span class="line-content">${escapeHtml(line)}</span></div>`;
              });
            }
          });

          oldDiffHtml = oldHtml;
          newDiffHtml = newHtml;

        } else if (type === 'added') {
          // '신규' 파일 내용 가져오기
          const res = await fetchFileContent(item.file_path);
          const content = res.content || `(내용 없음) \n오류: ${res.error || ''}`;
          newFileContent = content.split('\n').map((line, i) => 
            `<div class="line-wrapper"><span class="line-num">${i + 1}</span><span class="line-content">${escapeHtml(line)}</span></div>`
          ).join('');
          oldFileContent = ''; // 추가된 파일이므로 이전 내용은 없음
        } else if (type === 'deleted') {
          // '삭제' 파일 내용 가져오기
          const res = await fetchFileContent(item.file_path);
          const content = res.content || `(내용 없음) \n오류: ${res.error || ''}`;
          oldFileContent = content.split('\n').map((line, i) =>
            `<div class="line-wrapper"><span class="line-num">${i + 1}</span><span class="line-content">${escapeHtml(line)}</span></div>`
          ).join('');
          newFileContent = ''; // 삭제된 파일이므로 새 내용은 없음
        }
      } catch (error) {
        const errorMessage = `내용 로딩 실패: ${error.message}`;
        if (type === 'added') newFileContent = errorMessage;
        else if (type === 'deleted') oldFileContent = errorMessage;
        else { oldFileContent = errorMessage; newFileContent = errorMessage; }
      } finally {
        isContentLoading = false;
      }
    }
  }

  // 백엔드 API를 호출하여 파일 내용을 가져오는 함수
  async function fetchFileContent(filePath) {
    const response = await fetch(`${$rooturl}/execute-command/get-file-content`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ host, port, username, password, filePath }) });
    return response.json();
  }

  // 모달 팝업을 닫는 함수
  function closeModal() {
    showModal = false;
    oldDiffHtml = ''; newDiffHtml = '';
  }

  // 파일 경로와 목록 유형(1 또는 2)을 기반으로 파일의 상태를 반환하는 함수
  function getFileStatus(filePath, listType) {
    if (!comparisonResult) return 'unchanged';

    const transformedPath = transformPath(filePath);

    if (listType === 1) { // 왼쪽 목록 (비교 대상 1)
      if (comparisonResult.deleted.some(f => transformPath(f.file_path) === transformedPath)) return 'deleted';
      if (comparisonResult.modified.some(item => transformPath(item.old.file_path) === transformedPath)) return 'modified';
    } else { // 오른쪽 목록 (비교 대상 2)
      if (comparisonResult.added.some(f => transformPath(f.file_path) === transformedPath)) return 'added';
      if (comparisonResult.modified.some(item => transformPath(item.new.file_path) === transformedPath)) return 'modified';
    }
    return 'unchanged';
  }

  // 파일 상태에 따라 색상 이름을 반환하는 함수 (CSS 스타일링용)
  function getStatusColor(status) {
    switch (status) {
      case 'added': return 'green';
      case 'deleted': return 'red';
      case 'modified': return 'orange';
      default: return 'inherit';
    }
  }

  // XSS 공격 방지를 위해 HTML 태그를 이스케이프하는 헬퍼 함수
  function escapeHtml(str) {
    return str
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;');
  }

  // 스타일 추가
  const style = document.createElement('style');
  style.textContent = `
    .line-wrapper {
      display: flex;
      font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
    }
    .line-num {
      width: 40px;
      flex-shrink: 0; /* 줄어들지 않도록 설정 */
      text-align: right;
      padding-right: 8px;
      color: #9ca3af; /* gray-400 */
      user-select: none;
    }
    .line-content {
      flex-grow: 1; /* 남은 공간을 모두 차지 */
      white-space: pre-wrap; /* 공백 유지 및 자동 줄바꿈 */
      word-break: break-all; /* 긴 단어 강제 줄바꿈 */
    }
    .line-content.empty-line { 
      background-color: #f3f4f6; /* gray-100 */ 
    }
  `;
  document.head.appendChild(style);

</script>

<div class="container mx-auto p-4 lg:p-8 bg-gray-50 text-gray-800">
  <h2 class="text-2xl lg:text-3xl font-bold mb-6 text-gray-700">파일 변경 사항 비교</h2>

  <!-- SSH Connection Form -->
  <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4 mb-6 p-6 bg-white rounded-lg shadow-md">
    <div class="form-group">
      <label for="host" class="block text-sm font-medium text-gray-600 mb-1">호스트</label>
      <input id="host" type="text" bind:value={host} class="w-full p-2 border border-gray-300 rounded-md focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition" />
    </div>
    <div class="form-group">
      <label for="port" class="block text-sm font-medium text-gray-600 mb-1">포트</label>
      <input id="port" type="number" bind:value={port} class="w-full p-2 border border-gray-300 rounded-md focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition" />
    </div>
    <div class="form-group">
      <label for="username" class="block text-sm font-medium text-gray-600 mb-1">사용자명</label>
      <input id="username" type="text" bind:value={username} class="w-full p-2 border border-gray-300 rounded-md focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition" />
    </div>
    <div class="form-group">
      <label for="password" class="block text-sm font-medium text-gray-600 mb-1">비밀번호</label>
      <input id="password" type="password" bind:value={password} class="w-full p-2 border border-gray-300 rounded-md focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition" />
    </div>
  </div>

  <!-- Selection Form -->
  <div class="grid grid-cols-1 md:grid-cols-2 gap-x-8 gap-y-6 mb-6 p-6 bg-white rounded-lg shadow-md">
    <!-- Comparison Set 1 -->
   <div class="p-4 border rounded-lg">
        <h3 class="text-lg font-semibold mb-3 text-gray-700">비교 대상 1</h3>
        <div class="form-group mb-4">
            <label for="job-id-1" class="block text-sm font-medium text-gray-600 mb-1">Job ID 선택</label>
            <select id="job-id-1" bind:value={selectedJobId1} class="w-full p-2 border border-gray-300 rounded-md">
                <option value="">-- Job ID 선택 --</option>
                {#each allJobIds as job}
                <option value={job.job_id + '|' + job.table_name}>{job.job_id}</option>
                {/each}
            </select>
        </div>
    </div>
    <!-- Comparison Set 2 -->
    <div class="p-4 border rounded-lg">
        <h3 class="text-lg font-semibold mb-3 text-gray-700">비교 대상 2</h3>
        <div class="form-group mb-4">
            <label for="job-id-2" class="block text-sm font-medium text-gray-600 mb-1">Job ID 선택</label>
            <select id="job-id-2" bind:value={selectedJobId2} class="w-full p-2 border border-gray-300 rounded-md">
                <option value="">-- Job ID 선택 --</option>
                {#each allJobIds as job}
                <option value={job.job_id + '|' + job.table_name}>{job.job_id}</option>
                {/each}
            </select>
        </div>
    </div>
  </div>

  <div class="flex justify-center mb-6">
    <button on:click={compareJobs} disabled={isLoading || !selectedJobId1 || !selectedJobId2} class="bg-blue-600 text-white font-semibold py-2 px-6 rounded-lg hover:bg-blue-700 disabled:bg-gray-400 transition-colors shadow-md">
      {isLoading ? '비교 중...' : '비교하기'}
    </button>
  </div>

  <!-- Error Message -->
  {#if errorMessage}
    <div class="bg-red-100 border-l-4 border-red-500 text-red-700 p-4 my-4 rounded-md" role="alert">
      <p>{errorMessage}</p>
    </div>
  {/if}

  <!-- Comparison Result -->
  {#if comparisonResult}
    <!-- Filter and Legend Section -->
    <div class="my-4 p-4 bg-white rounded-lg shadow-md flex justify-between items-center">
      <div class="flex items-center space-x-4">
        <label for="filter-status" class="text-sm font-medium text-gray-600">필터:</label>
        <select id="filter-status" bind:value={filterStatus} class="p-2 border border-gray-300 rounded-md">
          <option value="all">전체</option>
          <!-- <option value="unchanged">동일</option> -->
          <option value="added">신규</option>
          <option value="modified">수정</option>
          <option value="deleted">삭제</option>
        </select>
      </div>
      <div class="text-sm">
        <span class="inline-flex items-center mr-3"><span class="w-3 h-3 rounded-full bg-green-500 mr-1.5"></span>신규</span>
        <span class="inline-flex items-center mr-3"><span class="w-3 h-3 rounded-full bg-red-500 mr-1.5"></span>삭제</span>
        <span class="inline-flex items-center mr-3"><span class="w-3 h-3 rounded-full bg-orange-500 mr-1.5"></span>수정</span>
      </div>
    </div>
    <div class="bg-white p-6 rounded-lg shadow-md">
      <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
        <!-- Left File List -->
        <div>
          <h3 class="text-lg font-semibold mb-3 text-gray-700">비교 대상 1: {selectedJobId1.split('|')[0]} ({filteredFiles1.length} / {comparisonResult.files1.length}개)</h3>
          <div class="overflow-auto border rounded-lg h-96">
            <table class="min-w-full divide-y divide-gray-200">
              <thead class="bg-gray-50 sticky top-0"><tr><th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">파일 경로</th><th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">크기</th></tr></thead>
              <tbody class="bg-white divide-y divide-gray-200">
                {#each filteredFiles1 as file (file.file_path)}
                  {@const status = getFileStatus(file.file_path, 1)}
                  <tr 
                    class="hover:bg-gray-50"
                    class:cursor-pointer={status !== 'unchanged'}
                    on:click={() => {
                      if (status === 'deleted') {
                        showDetails(file, 'deleted');
                      } else if (status === 'modified') {
                        const modifiedItem = comparisonResult.modified.find(item => transformPath(item.old.file_path) === transformPath(file.file_path));
                        if (modifiedItem) showDetails(modifiedItem, 'modified');
                      }
                    }}>
                    <td class="px-4 py-2 whitespace-nowrap text-sm font-mono" style="color: {getStatusColor(status)}">{file.file_path}</td>
                    <td class="px-4 py-2 whitespace-nowrap text-sm text-right">{file.file_size} bytes</td>
                  </tr>
                {:else}
                  <tr><td colspan="2" class="text-center py-4">파일이 없습니다.</td></tr>
                {/each}
              </tbody>
            </table>
          </div>
        </div>
        <!-- Right File List -->
        <div>
          <h3 class="text-lg font-semibold mb-3 text-gray-700">비교 대상 2: {selectedJobId2.split('|')[0]} ({filteredFiles2.length} / {comparisonResult.files2.length}개)</h3>
          <div class="overflow-auto border rounded-lg h-96">
            <table class="min-w-full divide-y divide-gray-200">
              <thead class="bg-gray-50 sticky top-0"><tr><th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">파일 경로</th><th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">크기</th></tr></thead>
              <tbody class="bg-white divide-y divide-gray-200">
                {#each filteredFiles2 as file (file.file_path)}
                  {@const status = getFileStatus(file.file_path, 2)}
                  <tr 
                    class="hover:bg-gray-50"
                    class:cursor-pointer={status !== 'unchanged'}
                    on:click={() => {
                      if (status === 'added') {
                        showDetails(file, 'added');
                      } else if (status === 'modified') {
                        const modifiedItem = comparisonResult.modified.find(item => transformPath(item.new.file_path) === transformPath(file.file_path));
                        if (modifiedItem) showDetails(modifiedItem, 'modified');
                      }
                    }}>
                    <td class="px-4 py-2 whitespace-nowrap text-sm font-mono" style="color: {getStatusColor(status)}">{file.file_path}</td>
                    <td class="px-4 py-2 whitespace-nowrap text-sm text-right">{file.file_size} bytes</td>
                  </tr>
                {:else}
                  <tr><td colspan="2" class="text-center py-4">파일이 없습니다.</td></tr>
                {/each}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  {/if}

  <!-- 상세 정보 모달 -->
  {#if showModal}
    <div class="fixed inset-0 bg-black bg-opacity-60 flex justify-center items-center z-50" on:click|self={closeModal}>
      <div class="bg-white rounded-lg shadow-xl w-[90%] h-[90%] transform transition-all flex flex-col">
        <div class="flex justify-between items-center border-b p-6">
          <h3 class="text-xl font-bold text-gray-800">
            {#if modalType === 'added'}추가된 파일 상세 정보
            {:else if modalType === 'deleted'}삭제된 파일 상세 정보
            {:else if modalType === 'modified'}수정된 파일 상세 정보
            {/if}
          </h3>
          <button on:click={closeModal} class="text-gray-500 hover:text-gray-800 text-2xl">&times;</button>
        </div>

        <div class="space-y-4 text-sm p-6 overflow-y-auto flex-grow">
          {#if modalType === 'added' || modalType === 'deleted' || modalType === 'modified'}
            <div><strong class="w-32 inline-block">파일 경로:</strong> <span class="font-mono">{modalType === 'modified' ? modalContent.new.file_path : modalContent.file_path}</span></div>
            <div class="grid grid-cols-1 md:grid-cols-2 gap-6 mt-4">
              <!-- 이전 정보 -->
              <div class="bg-red-50 p-4 rounded-lg flex flex-col">
                <h4 class="font-bold text-md mb-2 text-red-700">이전 정보</h4>
                {#if modalType === 'added'}
                  <div class="text-gray-500 italic h-full flex items-center justify-center">존재하지 않음</div>
                {:else}
                  {@const oldInfo = modalType === 'modified' ? modalContent.old : modalContent}
                  <div class="space-y-2 flex flex-col flex-grow">
                    <div><strong>파일 크기:</strong> {oldInfo.file_size} bytes</div>
                    <div><strong>수정 시간:</strong> {new Date(oldInfo.mod_time).toLocaleString()}</div>
                    <div><strong>CRC 값:</strong> {oldInfo.crc_value || 'N/A'}</div>
                    <h5 class="font-semibold mt-3 pt-2 border-t">파일 내용:</h5>
                    {#if isContentLoading}
                      <pre class="bg-gray-100 p-2 rounded text-xs flex-grow overflow-auto">로딩 중...</pre>
                    {:else}
                      <pre class="bg-gray-100 p-2 rounded text-xs flex-grow overflow-auto font-mono">{@html modalType === 'modified' ? oldDiffHtml : oldFileContent}</pre>
                    {/if}
                  </div>
                {/if}
              </div>
              <!-- 현재 정보 -->
              <div class="bg-green-50 p-4 rounded-lg flex flex-col">
                <h4 class="font-bold text-md mb-2 text-green-700">현재 정보</h4>
                {#if modalType === 'deleted'}
                  <div class="text-gray-500 italic h-full flex items-center justify-center">삭제됨</div>
                {:else}
                  {@const newInfo = modalType === 'modified' ? modalContent.new : modalContent}
                  <div class="space-y-2 flex flex-col flex-grow">
                    <div><strong>파일 크기:</strong> {newInfo.file_size} bytes</div>
                    <div><strong>수정 시간:</strong> {new Date(newInfo.mod_time).toLocaleString()}</div>
                    <div><strong>CRC 값:</strong> {newInfo.crc_value || 'N/A'}</div>
                    <h5 class="font-semibold mt-3 pt-2 border-t">파일 내용:</h5>
                    {#if isContentLoading}
                      <pre class="bg-gray-100 p-2 rounded text-xs flex-grow overflow-auto">로딩 중...</pre>
                    {:else}
                      <pre class="bg-gray-100 p-2 rounded text-xs flex-grow overflow-auto font-mono">{@html modalType === 'modified' ? newDiffHtml : newFileContent}</pre>
                    {/if}
                  </div>
                {/if}
              </div>
            </div>
          {/if}
        </div>

        <div class="text-right border-t p-6">
          <button on:click={closeModal} class="bg-gray-500 text-white font-semibold py-2 px-4 rounded-lg hover:bg-gray-600 transition-colors">
            닫기
          </button>
        </div>
      </div>
    </div>
  {/if}
</div>