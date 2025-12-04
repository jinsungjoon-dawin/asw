<script>
  import { authApps, isLogged, userid, intlMs } from "../aqtstore";
  const imgUrl = new URL("/images/Logo.png", import.meta.url).href;
  import { Buffer } from 'buffer';
  import {rooturl} from "../aqtstore";
  import { onMount } from "svelte";
	let showModal = false;
	let usrid = "";
  let usridInput;
	let password = "";
  let passwordInput;
	let error = "";
  let usridChk = ""; 

  async function login() {
  // const chk = await getCheckPass(password);
  const res = await fetch($rooturl + "/logonchk?pass=" + password);
  let data = await res.json();

		if (data.chk) {
			$isLogged = 2;
			if (error) error = "";
		} else {
			error = "비밀번호가 맞지않습니다.";
		}
	}

	async function login3() {
    if(usrid === ""){
      alert("아이디를 입력하세요.");
      usridInput.focus();
      return;
    }else if(password === ""){
      alert("비밀번호를 입력하세요.");
      passwordInput.focus();
      return;
    }
		if (password === "aqtuser") {
			showModal = true;
			return;
		}
		fetch($rooturl + "/logonchk", {
			method: "POST",
			headers: {
				"Content-Type": "application/json",
			},
			body: JSON.stringify({
				pass:
					Math.floor(Math.random() * 100)
						.toString()
						.padStart(2, "0") +
					Buffer.from(password, "utf8").toString("base64"),
				usrid:
					Math.floor(Math.random() * 10) +
					Buffer.from(usrid, "utf8").toString("base64"),
			}),
		})
    .then(async (res) => {
				const data = await res.json();
        if (!res.ok) {
          alert(data.message || 'API Error');
        }
				if (data.hg == 0) error = "허가되지않은 IP.";
				else if (data.chk) {
					$isLogged = data.admin == "1" ? 2 : 1;
					$authApps = data.apps;
					$userid = usrid;
					//					getTcodelist() ;
					if (error) error = "";
				} else {
					error = "비밀번호가 맞지않습니다.";
          alert(error);
				}
			})
			.catch((err) => {
				error = err.message;
				alert(error);
			});
	}

	async function login2() {
		// $isLogged = 1;
    localStorage.setItem('usrid', usrid);
    localStorage.setItem('usridChk', usridChk);
    isLogged.set(true);
	}

  

  
  function toggleModal() {
    showModal = !showModal;
  }

  let opass = "";
  let opassInput;
  let npass1 = "";
  let npass1Input;
  let npass2 = "";
  let npass2Input;
  
  async function changepass() {
    if(opass === ""){
      alert("변경전 비밀번호");
      opassInput.focus();
      return;
    }else if(npass1 === ""){
      alert("변경할 비밀번호를 입력하세요.");
      npass1Input.focus();
      return;
    }else if(npass2 === ""){
      alert("변경확인 비밀번호를 입력하세요.");
      npass2Input.focus();
      return;
    }
    if (npass1 !== npass2) {
      alert("비밀번호 확인값이 다릅니다.");
      return ;
    }
    if (! /^(?=.*[a-zA-Z])(?=.*[!@#$%^*+=-])(?=.*[0-9]).{8,}$/.test(npass1)) {
      alert("영숫자 및 특수문자를 포함하여 8자리이상이어야 합니다.");
      return ;
    }
    fetch($rooturl + "/logonchk/cp", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        opass:
          Math.floor(Math.random() * 100)
            .toString()
            .padStart(2, "0") + Buffer.from(opass, "utf8").toString("base64"),
        pass:
          Math.floor(Math.random() * 100)
            .toString()
            .padStart(2, "0") + Buffer.from(npass1, "utf8").toString("base64"),
        usrid:
          Math.floor(Math.random() * 10) +
          Buffer.from(usrid, "utf8").toString("base64"),
      }),
    })
      .then(async (res) => {
        let rmsg = await res.json();
        if (res.status < 400) {
          alert(rmsg.message);
          toggleModal();
          // dialog.close();
          // return ( ! rmsg.err );
        }
      })
      .catch((err) => {
        alert(err.message);
      });
      return false ;
  }

  let rdata = [];

  async function getData() {
    try{
    const res = await fetch($rooturl + "/dashboard");
    if (res.ok){
      return await res.json();
    }
    else
      throw new Error(res.statusText);
    }catch(error){
      alert(error)
    }
  }

  onMount(async() => {
    
    rdata = await getData();
    intlMs.set(rdata[0].intl * 1000); // 타이머 주기 설정값 store 저장 $intlMs 사용
      
    if(localStorage.getItem('usridChk') === "true"){
      usridChk = true;
      usrid = localStorage.getItem('usrid');
    }
    
  })


  
  </script>
  
  <style>
    /* 추가적인 커스터마이징이 필요하면 여기 추가 */
  </style>
    <div class="min-h-screen bg-gray-100 flex items-center flex-start  justify-center " >
    <div class="w-full max-w-md p-8 bg-white rounded-lg shadow-md ">
      <!-- <div class="flex justify-center">
        <img src="src/img/logo4.png" alt="Logo" class="p-5 object-contain" />
      </div>
      <div class="flex justify-center">
        <img src="src/img/logo5.png" alt="Logo" class="p-5 object-contain" />
      </div> -->
      <!-- <div class="flex justify-center">
        <img src="src/img/logo6.png" alt="Logo" class="p-5 object-contain" />
      </div>
      <div class="flex justify-center">
        <img src="src/img/logo7.png" alt="Logo" class="p-5 object-contain" />
      </div>
      <div class="flex justify-center">
        <img src="src/img/logo8.png" alt="Logo" class="p-5 object-contain" />
      </div> -->
      
      <!-- <div class="flex justify-center font-bold " style="color: #3b82f6;">
        <img src="src/img/logo9.png" alt="Logo" class="p-5 object-contain" />
      </div> -->
      <div class="flex justify-center items-end font-bold text-4xl  font-serif text-blue-500 h-24">
         T&T Boards
      </div>
      <div class="flex justify-center font-bold text-[xx-small] italic font-serif ml-40 h-10">
         Test & Transition
      </div>
      <div class="w-full text-xl font-semibold text-center text-gray-700 mb-6">{rdata[0]?.pjtname}</div>
      
      <!-- 로그인 폼 -->
      <form on:submit|preventDefault={() => {}} >
        <!-- 이메일 입력 -->
        <div class="mb-4">
          <label for="email" class="block text-sm font-medium text-gray-600">아이디</label>
          <input
            type="text"
            id="id"
            class="w-full px-4 py-2 mt-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
            placeholder="아이디를 입력하세요."
            bind:value={usrid}
            bind:this={usridInput}
          />
        </div>
        
        <!-- 비밀번호 입력 -->
        <div class="mb-6">
          <label for="password" class="block text-sm font-medium text-gray-600">비밀번호</label>
          <input
            type="password"
            id="password"
            class="w-full px-4 py-2 mt-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
            placeholder="비밀번호를 입력하세요."
            bind:value={password}
            bind:this={passwordInput}
          />
        </div>
        
        <!-- 로그인 버튼 -->
        <button class="w-full py-2 bg-blue-500 text-white font-semibold rounded-lg hover:bg-blue-600 focus:outline-none" on:click={login3} >
          로그인
        </button>
      </form>
      <!-- 비밀번호 재설정 링크 -->
      <div class="mt-4 flex justify-between">
        <div><label for="usridChk">계정저장</label> <input type="checkbox" id="usridChk" bind:checked={usridChk}></div>
        <a href="#" class="text-sm text-blue-500 hover:underline" on:click={toggleModal}>비밀번호 변경</a>
      </div>
  
     
      <div class="flex justify-end h-24">
        &nbsp;
         <img src="/img/DAWINICT_logo.png" alt="Logo" class="w-20 h-20 object-contain" />
      </div>
      
    </div>
  </div>
  {#if showModal}
  <div class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center">
    <div class="bg-white p-6 rounded-lg shadow-lg w-96">
      <h3 class="text-xl font-semibold mb-4">비밀번호 변경</h3>

      <div class="mb-4">
        <label for="email" class="block text-sm font-medium text-gray-600">변경전 비밀번호</label>
        <input
          type="password"
          id="id"
          class="w-full px-4 py-2 mt-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="아이디를 입력하세요."
          bind:value={opass}
          bind:this={opassInput}
        />
      </div>
      
      <!-- 비밀번호 입력 -->
      <div class="mb-6">
        <label for="password" class="block text-sm font-medium text-gray-600">변경할 비밀번호</label>
        <input
          type="password"
          id="password"
          class="w-full px-4 py-2 mt-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="비밀번호를 입력하세요."
          bind:value={npass1} 
          bind:this={npass1Input}/>
      </div>
      <div class="mb-6">
        <label for="password" class="block text-sm font-medium text-gray-600">변경확인 비밀번호</label>
        <input
          type="password"
          id="password"
          class="w-full px-4 py-2 mt-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="확인 비밀번호를 입력하세요."
          bind:value={npass2} 
          bind:this={npass2Input}/>
      </div>
      <div class="mt-4 flex justify-between">
        <button on:click={toggleModal} class="px-4 py-2 bg-gray-300 text-gray-700 rounded-md">
          취소
        </button>
        <button class="px-4 py-2 bg-blue-500 text-white rounded-md hover:bg-blue-600" on:click={changepass}>
          비밀번호 재설정
        </button>
      </div>
    </div>
  </div>
{/if}